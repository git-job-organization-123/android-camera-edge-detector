#include <jni.h>
#include <android/log.h>
#include <GLES3/gl3.h>
#include <array>

#include <opencv2/core.hpp> // OpenCV core
#include <opencv2/imgproc.hpp> // OpenCV COLOR_
#include <opencv2/features2d.hpp> // OpenCV fast feature detector

using namespace cv;

enum PreviewMode {
  DETECT_PREVIEW_EDGES_WHITE,
  DETECT_PREVIEW_EDGES_RED,
  DETECT_PREVIEW_EDGES_GREEN,
  DETECT_PREVIEW_EDGES_BLUE,
  DETECT_PREVIEW_EDGES_GRAYSCALE,
  DETECT_PREVIEW_EDGES_WHITE_WITH_BACKGROUND,
  DETECT_EDGES_FAST,
  DETECT_EDGES_FAST_LINES,
};

PreviewMode previewMode;
PreviewMode previousPreviewMode;

GLuint ibo;
GLuint vbo;

int cameraWidth;
int cameraHeight;

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
#include "renderer.cpp"
#include "renderer_red_squares.cpp"
#include "renderer_red_lines.cpp"
#include "renderer_texture.cpp"

bool initialized;

GLuint gProgram;
GLuint gTextureProgram;

// Shader program currently in use
GLuint currentProgram;

void setupDefaults() {
  // Default preview mode
  previewMode = PreviewMode::DETECT_PREVIEW_EDGES_WHITE;
  previousPreviewMode = previewMode;
}

Detector *currentDetector = nullptr;

Detector_Edges_Image_Red *redEdgesImageDetector;
Detector_Edges_Image_Green *greenEdgesImageDetector;
Detector_Edges_Image_Blue *blueEdgesImageDetector;
Detector_Edges_Image_White *whiteEdgesImageDetector;
Detector_Edges_Image_Grayscale *grayscaleEdgesImageDetector;
Detector_Edges_Image_Background *backgroundEdgesImageDetector;
Detector_Edges_Points *pointsEdgesDetector;

void setupDetectors() {
  redEdgesImageDetector = new Detector_Edges_Image_Red();
  greenEdgesImageDetector = new Detector_Edges_Image_Green();
  blueEdgesImageDetector = new Detector_Edges_Image_Blue();
  whiteEdgesImageDetector = new Detector_Edges_Image_White();
  grayscaleEdgesImageDetector = new Detector_Edges_Image_Grayscale();
  backgroundEdgesImageDetector = new Detector_Edges_Image_Background();
  pointsEdgesDetector = new Detector_Edges_Points();
}

void setDetector(Detector *detector) {
  currentDetector = detector;
}

void updateDetector() {
  if (currentDetector != nullptr) {
    currentDetector->clear();
  }

  switch (previewMode) {
    case PreviewMode::DETECT_PREVIEW_EDGES_WHITE:
      currentDetector = whiteEdgesImageDetector;
      break;
    case PreviewMode::DETECT_PREVIEW_EDGES_RED:
      currentDetector = redEdgesImageDetector;
      break;
    case PreviewMode::DETECT_PREVIEW_EDGES_GREEN:
      currentDetector = greenEdgesImageDetector;
      break;
    case PreviewMode::DETECT_PREVIEW_EDGES_BLUE:
      currentDetector = blueEdgesImageDetector;
      break;
    case PreviewMode::DETECT_PREVIEW_EDGES_GRAYSCALE:
      currentDetector = grayscaleEdgesImageDetector;
      break;
    case PreviewMode::DETECT_PREVIEW_EDGES_WHITE_WITH_BACKGROUND:
      currentDetector = backgroundEdgesImageDetector;
      break;
    case PreviewMode::DETECT_EDGES_FAST:
      currentDetector = pointsEdgesDetector;
      break;
    case PreviewMode::DETECT_EDGES_FAST_LINES:
      currentDetector = pointsEdgesDetector;
      break;
  }

  currentDetector->init();
}

Renderer *renderer;

void setRenderer(Renderer *renderer_) {
  renderer = renderer_;
}

Renderer_Red_Squares *squaresRenderer;
Renderer_Red_Lines *linesRenderer;
Renderer_Texture *textureRenderer;

void setupRenderers() {
  squaresRenderer = new Renderer_Red_Squares(gProgram);
  linesRenderer = new Renderer_Red_Lines(gProgram);
  textureRenderer = new Renderer_Texture(gTextureProgram);
}

void updateRenderer() {
  if (previewMode == PreviewMode::DETECT_EDGES_FAST) {
    setRenderer(squaresRenderer);
  }
  else if (previewMode == PreviewMode::DETECT_EDGES_FAST_LINES) {
    setRenderer(linesRenderer);
  }
  else {
    setRenderer(textureRenderer);
  }
}

void updatePreviewMode() {
  if (previewMode == previousPreviewMode) {
    return;
  }
  
  updateDetector();
  updateRenderer();

  previousPreviewMode = previewMode;
}

const char* gVertexShader = R"(#version 300 es
  layout(location = 0) in vec2 vPosition;

  void main() {
    gl_Position = vec4(vPosition, 0.0, 1.0);
  }
)";

const char* gFragmentShader = R"(#version 300 es
  precision mediump float;
  out vec4 fragColor;

  void main() {
    fragColor = vec4(1.0, 0.0, 0.0, 1.0); // default color is red 
  }
)";

const char* gTextureVertexShader = R"(#version 300 es
  layout(location = 0) in vec2 vPosition;
  layout(location = 1) in vec2 vTexCoord;
  out vec2 texCoord;

  void main() {
    gl_Position = vec4(vPosition, 0.0, 1.0);
    texCoord = vTexCoord;
  }
)";

const char* gTextureFragmentShader = R"(#version 300 es
  precision mediump float;
  in vec2 texCoord;
  uniform sampler2D uTexture;
  out vec4 fragColor;

  void main() {
    fragColor = texture(uTexture, texCoord);
  }
)";

GLuint loadShader(GLenum type, const char *shaderSrc) {
  GLuint shader;
  GLint compiled;
  shader = glCreateShader(type);

  if (shader == 0) {
    return 0;
  }

  glShaderSource(shader, 1, &shaderSrc, NULL);
  glCompileShader(shader);
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (!compiled) {
    GLint infoLen = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
    if (infoLen > 1) {
      char *infoLog = (char *)malloc(sizeof(char) * infoLen);
      glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
      __android_log_print(ANDROID_LOG_ERROR, "edgedetector", "Error compiling shader:\n%s\n", infoLog);
      free(infoLog);
    }
    glDeleteShader(shader);
    return 0;
  }
  return shader;
}

GLuint createProgram(const char *vertexSource, const char *fragmentSource) {
  GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexSource);
  if (!vertexShader) {
    return 0;
  }

  GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentSource);
  if (!fragmentShader) {
    return 0;
  }

  GLuint program = glCreateProgram();
  if (program == 0) {
    return 0;
  }

  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);
  glLinkProgram  (program);
  GLint linkStatus = GL_FALSE;
  glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
  if (linkStatus != GL_TRUE) {
    GLint bufLength = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
    if (bufLength) {
      char *buf = (char *)malloc(bufLength);
      if (buf) {
        glGetProgramInfoLog(program, bufLength, NULL, buf);
        __android_log_print(ANDROID_LOG_ERROR, "edgedetector", "Could not link program:\n%s\n", buf);
        free(buf);
      }
    }
    glDeleteProgram(program);
    program = 0;
  }
  return program;
}

void setupGraphics(int width, int height) {
  gProgram = createProgram(gVertexShader, gFragmentShader);
  if (!gProgram) {
    return;
  }

  gTextureProgram = createProgram(gTextureVertexShader, gTextureFragmentShader);
  if (!gTextureProgram) {
    return;
  }

  // Default program
  glUseProgram(gTextureProgram);
  currentProgram = gTextureProgram;

  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ibo);

  glViewport(0, 0, width, height);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void detectFrame(unsigned char* nv21ImageData) {  
  currentDetector->setImageData(nv21ImageData);
  currentDetector->detect();
  currentDetector->clearImage();
}

void renderFrame() {
  if (currentProgram != renderer->program) {
    glUseProgram(renderer->program);
    currentProgram = renderer->program;
  }

  if (previewMode == PreviewMode::DETECT_EDGES_FAST 
   || previewMode == PreviewMode::DETECT_EDGES_FAST_LINES) { // Image points
    renderer->setKeypoints(currentDetector->keypoints);
  }
  else { // Image edges
    renderer->setImageData(currentDetector->processedImage.data);
  }

  renderer->draw();
}

extern "C" {
  JNIEXPORT void JNICALL Java_com_app_edgedetector_MyGLSurfaceView_init(JNIEnv *env, jobject obj,  jint width, jint height);
  JNIEXPORT void JNICALL Java_com_app_edgedetector_MyGLSurfaceView_setCameraSettings(JNIEnv* env, jobject obj, int32_t width, int32_t height);
  JNIEXPORT void JNICALL Java_com_app_edgedetector_MyGLSurfaceView_draw(JNIEnv *env, jobject obj);
  JNIEXPORT void JNICALL Java_com_app_edgedetector_MyGLSurfaceView_processImageBuffers(JNIEnv* env, jobject obj, jobject y, int ySize, int yPixelStride, int yRowStride, jobject u, int uSize, int uPixelStride, int uRowStride, jobject v, int vSize, int vPixelStride, int vRowStride);
  JNIEXPORT void JNICALL Java_com_app_edgedetector_MyGLSurfaceView_touch(JNIEnv *env, jobject obj, int previewMode);
};

JNIEXPORT void JNICALL Java_com_app_edgedetector_MyGLSurfaceView_init(JNIEnv *env, jobject obj,  jint width, jint height) {
  setupDefaults();

  setupGraphics(width, height);

  setupDetectors();
  setupRenderers();

  updateDetector();
  updateRenderer();

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
                                                                             jobject obj,
                                                                             int previewMode_) {
  previewMode = static_cast<PreviewMode>(previewMode_); // Set preview mode
  updatePreviewMode();
}
