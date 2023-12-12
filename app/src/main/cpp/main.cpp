#include <jni.h>
#include <android/log.h>
#include <GLES3/gl3.h>
#include <array>

#include <opencv2/core.hpp> // OpenCV core
#include <opencv2/imgproc.hpp> // OpenCV COLOR_
#include <opencv2/features2d.hpp> // OpenCV fast feature detector

using namespace cv;

int cameraWidth;
int cameraHeight;

#include "renderer.cpp"
#include "renderer_red_squares.cpp"
#include "renderer_red_lines.cpp"
#include "renderer_texture.cpp"
#include "detector.cpp"
#include "detector_edges.cpp"
#include "detector_edges_image.cpp"
#include "detector_edges_image_color.cpp"
#include "detector_edges_image_red.cpp"
#include "detector_edges_image_green.cpp"
#include "detector_edges_image_blue.cpp"
#include "detector_edges_image_white.cpp"
#include "detector_edges_image_grayscale.cpp"
#include "detector_edges_image_background.cpp"
#include "detector_edges_points.cpp"

bool initialized;

Detector_Edges_Image_Red *redEdgesImageDetector;
Detector_Edges_Image_Green *greenEdgesImageDetector;
Detector_Edges_Image_Blue *blueEdgesImageDetector;
Detector_Edges_Image_White *whiteEdgesImageDetector;
Detector_Edges_Image_Grayscale *grayscaleEdgesImageDetector;
Detector_Edges_Image_Background *backgroundEdgesImageDetector;
Detector_Edges_Points *pointsEdgesDetector;

Renderer_Red_Squares *redSquaresRenderer;
Renderer_Red_Lines *redLinesRenderer;
Renderer_Texture *textureRenderer;

struct PreviewMode {
  Detector *detector;
  Renderer *renderer;

  PreviewMode(Detector *detector_, Renderer *renderer_) {
    detector = detector_;
    renderer = renderer_;
    detector_->setRenderer(renderer_);
  }
};

std::vector<PreviewMode*> previewModes;

PreviewMode *currentPreviewMode;
int currentPreviewModeIndex = 0;

bool changeShaderProgramOnNextDraw = false;

void setupDetectors() {
  redEdgesImageDetector = new Detector_Edges_Image_Red();
  greenEdgesImageDetector = new Detector_Edges_Image_Green();
  blueEdgesImageDetector = new Detector_Edges_Image_Blue();
  whiteEdgesImageDetector = new Detector_Edges_Image_White();
  grayscaleEdgesImageDetector = new Detector_Edges_Image_Grayscale();
  backgroundEdgesImageDetector = new Detector_Edges_Image_Background();
  pointsEdgesDetector = new Detector_Edges_Points();
}

void setupRenderers() {
  redSquaresRenderer = new Renderer_Red_Squares();
  redLinesRenderer = new Renderer_Red_Lines();
  textureRenderer = new Renderer_Texture();
}

// Set up detector renderer pairs
void setupPreviewModes() {
  previewModes.push_back(new PreviewMode(whiteEdgesImageDetector, textureRenderer));
  previewModes.push_back(new PreviewMode(redEdgesImageDetector, textureRenderer));
  previewModes.push_back(new PreviewMode(greenEdgesImageDetector, textureRenderer));
  previewModes.push_back(new PreviewMode(blueEdgesImageDetector, textureRenderer));
  previewModes.push_back(new PreviewMode(grayscaleEdgesImageDetector, textureRenderer));
  previewModes.push_back(new PreviewMode(backgroundEdgesImageDetector, textureRenderer));
  previewModes.push_back(new PreviewMode(pointsEdgesDetector, redSquaresRenderer));
  previewModes.push_back(new PreviewMode(pointsEdgesDetector, redLinesRenderer));
}

void selectPreviewModeAtIndex(const int index) {
  if (currentPreviewMode != nullptr) {
    // currentPreviewMode->detector->clear();
    currentPreviewMode->renderer->clear();
  }

  currentPreviewModeIndex = index;
  currentPreviewMode = previewModes.at(currentPreviewModeIndex);

  currentPreviewMode->detector->init();
  currentPreviewMode->renderer->init();

  // Change shader progran on next draw
  changeShaderProgramOnNextDraw = true;
}

void selectNextPreviewMode() {
  ++currentPreviewModeIndex;

  if (currentPreviewModeIndex >= previewModes.size()) {
    // Select preview mode at start
    currentPreviewModeIndex = 0;
  }

  selectPreviewModeAtIndex(currentPreviewModeIndex);
}

void setupGraphics(int width, int height) {
  redSquaresRenderer->setupProgram();  
  redLinesRenderer->setupProgram();  
  textureRenderer->setupProgram(); 

  // Default program
  glUseProgram(currentPreviewMode->renderer->program);

  glViewport(0, 0, width, height);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void detectFrame(unsigned char* nv21ImageData) {  
  if (changeShaderProgramOnNextDraw) {
    return; // Prevent crash
  }

  currentPreviewMode->detector->setImageData(nv21ImageData);
  currentPreviewMode->detector->detect();
  currentPreviewMode->detector->updateRendererData();
  currentPreviewMode->detector->clearImage();
}

void renderFrame() {
  if (changeShaderProgramOnNextDraw) {
    glUseProgram(currentPreviewMode->renderer->program);
    changeShaderProgramOnNextDraw = false;
    return; // Prevent crash
  }

  currentPreviewMode->renderer->draw();
}

extern "C" {
  JNIEXPORT void JNICALL Java_com_app_edgedetector_MyGLSurfaceView_init(JNIEnv *env, jobject obj,  jint width, jint height);
  JNIEXPORT void JNICALL Java_com_app_edgedetector_MyGLSurfaceView_setCameraSettings(JNIEnv* env, jobject obj, int32_t width, int32_t height);
  JNIEXPORT void JNICALL Java_com_app_edgedetector_MyGLSurfaceView_draw(JNIEnv *env, jobject obj);
  JNIEXPORT void JNICALL Java_com_app_edgedetector_MyGLSurfaceView_processImageBuffers(JNIEnv* env, jobject obj, jobject y, int ySize, int yPixelStride, int yRowStride, jobject u, int uSize, int uPixelStride, int uRowStride, jobject v, int vSize, int vPixelStride, int vRowStride);
  JNIEXPORT void JNICALL Java_com_app_edgedetector_MyGLSurfaceView_touch(JNIEnv *env, jobject obj);
};

JNIEXPORT void JNICALL Java_com_app_edgedetector_MyGLSurfaceView_init(JNIEnv *env, jobject obj,  jint width, jint height) {
  // Set up renderer and detector classes
  setupRenderers();
  setupDetectors();

  // Set up list of preview modes
  setupPreviewModes();

  // Select first preview mode
  selectPreviewModeAtIndex(0);

  // Set up graphics
  setupGraphics(width, height);

  initialized = true;
}

JNIEXPORT void JNICALL Java_com_app_edgedetector_MyGLSurfaceView_setCameraSettings(JNIEnv* env,
                                                    jobject obj,
                                                    int32_t width,
                                                    int32_t height) {
  cameraWidth = width;
  cameraHeight = height;
}

JNIEXPORT void JNICALL Java_com_app_edgedetector_MyGLSurfaceView_draw(JNIEnv *env, jobject obj) {
  renderFrame();
}

JNIEXPORT void JNICALL Java_com_app_edgedetector_MyGLSurfaceView_processImageBuffers(JNIEnv* env,
                                                                jobject obj,
                                                                jobject y,
                                                                int ySize,
                                                                int yPixelStride,
                                                                int yRowStride,
                                                                jobject u,
                                                                int uSize,
                                                                int uPixelStride,
                                                                int uRowStride,
                                                                jobject v,
                                                                int vSize,
                                                                int vPixelStride,
                                                                int vRowStride) {
  if (!initialized) {
    // Make sure that init is called before processImageBuffers to prevent crash
    return;
  }

  try {
    // Get the address of the underlying memory of the ByteBuffer objects
    unsigned char* yData = (unsigned char*)env->GetDirectBufferAddress(y);
    unsigned char* uData = (unsigned char*)env->GetDirectBufferAddress(u);
    unsigned char* vData = (unsigned char*)env->GetDirectBufferAddress(v);

    unsigned char* nv21ImageData = (unsigned char*)malloc(ySize + uSize + vSize);

    int yIndex = 0;
    int uvIndex = ySize;

    // Use memcpy to process YUV_420_888 image
    // Note: this process will only be effective if the source and destination data are both aligned 
    // and the size is a multiple of 4
    for (int i = 0; i < cameraHeight; i++) {
      int ySrcIndex = i * yRowStride;
      int uvSrcIndex = i / 2 * uRowStride;

      int ySizeWidth = cameraWidth * yPixelStride;

      // Use memcpy to copy the Y data
      memcpy(nv21ImageData + yIndex, yData + ySrcIndex, ySizeWidth);
      yIndex += cameraWidth * yPixelStride;

      if (i % 2 == 0) {
        // Use memcpy to copy the U and V data
        memcpy(nv21ImageData + uvIndex, uData + uvSrcIndex, cameraWidth * uPixelStride / 2);
        uvIndex += cameraWidth * uPixelStride / 2;
        memcpy(nv21ImageData + uvIndex, uData + uvSrcIndex, cameraWidth * vPixelStride / 2);
        uvIndex += cameraWidth * vPixelStride / 2;
      }
    }

    // Detect edges from image
    detectFrame(nv21ImageData);

    // Free NV21 image from memory
    free(nv21ImageData);

    env->DeleteLocalRef(y);
    env->DeleteLocalRef(u);
    env->DeleteLocalRef(v);
  }
  catch (const std::exception& e) {
    __android_log_print(ANDROID_LOG_DEBUG, "edgedetector", "Error: %s", e.what());
    env->DeleteLocalRef(y);
    env->DeleteLocalRef(u);
    env->DeleteLocalRef(v);
  }
}

JNIEXPORT void JNICALL Java_com_app_edgedetector_MyGLSurfaceView_touch(JNIEnv* env,
                                                                       jobject obj) {
  selectNextPreviewMode();
}
