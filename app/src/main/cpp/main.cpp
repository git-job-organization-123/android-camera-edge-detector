#include <jni.h>
#include <android/log.h>
#include <GLES3/gl3.h>
#include <array>

#include <opencv2/core.hpp> // OpenCV core
#include <opencv2/imgproc.hpp> // OpenCV COLOR_
#include <opencv2/features2d.hpp> // OpenCV fast feature detector

using namespace cv;

bool initialized;

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

// Default preview mode
PreviewMode previewMode = PreviewMode::DETECT_PREVIEW_EDGES_WHITE;
PreviewMode previousPreviewMode = previewMode;

int cameraWidth;
int cameraHeight;

// NV21 image from Android device
cv::Mat grayImage;

// Edge image
cv::Mat processedImage;

// Fast feature detector
cv::Ptr<cv::Feature2D> featureDetector;

// Detected points (red squares)
std::vector<cv::KeyPoint> keypoints;

GLuint gProgram;
GLuint gTextureProgram;

// Change to this shader program on next tick
GLuint changeToProgram;

// Shader program currently in use
GLuint currentProgram;

// Red square arrays
GLuint gvPositionHandle;
GLuint ibo;
GLuint vbo;

// Add attribute arrays for the texture coordinates and a handle for the texture uniform
GLuint gvtPositionHandle;
GLint gvtTexCoordHandle;
GLint gutTextureHandle;
GLuint gTexture;

void updateDetector() {
  if (previewMode == PreviewMode::DETECT_EDGES_FAST 
   || previewMode == PreviewMode::DETECT_EDGES_FAST_LINES) { // Red squares and red lines
    cv::Ptr<cv::FastFeatureDetector> fast = cv::FastFeatureDetector::create();
    fast->setThreshold(12); // 10 = default
    featureDetector = fast;
  }
}

void setupDetector() {
  updateDetector();
}

void updatePreviewMode() {
  if (previewMode == previousPreviewMode) {
    return;
  }

  if (previewMode == PreviewMode::DETECT_EDGES_FAST
   || previewMode == PreviewMode::DETECT_EDGES_FAST_LINES) {
    changeToProgram = gProgram; // Red squares and lines shader
  }
  else {
    changeToProgram = gTextureProgram; // Texture shader
  }
  
  updateDetector();

  previousPreviewMode = previewMode;
}

void detectKeypoints() {
  // Create a list to hold the keypoints
  featureDetector->detect(grayImage, keypoints);
}

void dilate(cv::Mat* image, int x, int y) {
  cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5,5));
  cv::dilate(*image, *image, kernel);
}

void colorEdges(cv::Mat* edgeImage) {
  cv::Mat colorEdges;
  cv::cvtColor(*edgeImage, colorEdges, cv::COLOR_GRAY2BGR);
  std::vector<cv::Mat> channels;
  cv::split(colorEdges, channels);
  channels[0] = channels[1] = channels[2] = 0;

  if (previewMode == PreviewMode::DETECT_PREVIEW_EDGES_RED) {
    channels[2] = *edgeImage;
  }
  else if (previewMode == PreviewMode::DETECT_PREVIEW_EDGES_GREEN) {
    channels[1] = *edgeImage;
  }
  else if (previewMode == PreviewMode::DETECT_PREVIEW_EDGES_BLUE) {
    channels[0] = *edgeImage;
  }

  cv::merge(channels, colorEdges);

  *edgeImage = colorEdges;
}

void detectEdges(cv::Mat* edgeImage) {
  cv::Canny(grayImage, *edgeImage, 80, 90);

  if (previewMode == PreviewMode::DETECT_PREVIEW_EDGES_RED
   || previewMode == PreviewMode::DETECT_PREVIEW_EDGES_GREEN
   || previewMode == PreviewMode::DETECT_PREVIEW_EDGES_BLUE) {
    // Make edges thicker
    dilate(edgeImage, 1, 1);

    colorEdges(edgeImage);
  }
  else if (previewMode == PreviewMode::DETECT_PREVIEW_EDGES_GRAYSCALE) {
    // Make edges thicker
    dilate(edgeImage, 20, 20);

    // Apply pixels from original image to edge image
    grayImage.copyTo(*edgeImage, *edgeImage);
  }
  else if (previewMode == PreviewMode::DETECT_PREVIEW_EDGES_WHITE_WITH_BACKGROUND) {
    cv::cvtColor(*edgeImage, *edgeImage, cv::COLOR_BGR2RGB);

    // Copy edges to preview image
    cv::addWeighted(*edgeImage, 0.5, grayImage, 0.5, 0, *edgeImage);
  }
}

void detect() {
  if (previewMode == PreviewMode::DETECT_EDGES_FAST
   || previewMode == PreviewMode::DETECT_EDGES_FAST_LINES) {  // Fast detector red squares and fast detector red lines
    detectKeypoints();
  }
  else if (previewMode == PreviewMode::DETECT_PREVIEW_EDGES_WHITE
        || previewMode == PreviewMode::DETECT_PREVIEW_EDGES_RED
        || previewMode == PreviewMode::DETECT_PREVIEW_EDGES_GREEN
        || previewMode == PreviewMode::DETECT_PREVIEW_EDGES_BLUE
        || previewMode == PreviewMode::DETECT_PREVIEW_EDGES_GRAYSCALE
        || previewMode == PreviewMode::DETECT_PREVIEW_EDGES_WHITE_WITH_BACKGROUND) { // Preview edges
    detectEdges(&processedImage);
    cv::cvtColor(processedImage, processedImage, cv::COLOR_BGR2RGB);
  }
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

// Red square
GLfloat gSquareVertices[] = {
  0.5f, 0.5f, -0.5f, // top right
  0.5f, -0.5f, -0.5f, // bottom right
  -0.5f, -0.5f, -0.5f, // bottom left
  -0.5f, 0.5f, -0.5f, // top left
};

// Texture
GLfloat gTextureVertices[] = {
  1.0f, 1.0f, -0.5f, 0.0f, 0.0f,  // top right
  1.0f, -1.0f, -0.5f, 1.0f, 0.0f,  // bottom right
  -1.0f, -1.0f, -0.5f, 1.0f, 1.0f,  // bottom left
  -1.0f, 1.0f, -0.5f, 0.0f, 1.0f,  // top left
};

const GLushort gIndices[] = {
  0, 1, 2,
  2, 3, 0
};

void setupGraphics(int width, int height) {
  gProgram = createProgram(gVertexShader, gFragmentShader);
  if (!gProgram) {
    return;
  }

  gvPositionHandle = glGetAttribLocation(gProgram, "vPosition");

  gTextureProgram = createProgram(gTextureVertexShader, gTextureFragmentShader);
  if (!gTextureProgram) {
    return;
  }

  gvtPositionHandle = glGetAttribLocation(gTextureProgram, "vPosition");
  gvtTexCoordHandle = glGetAttribLocation(gTextureProgram, "vTexCoord");
  gutTextureHandle = glGetUniformLocation(gTextureProgram, "uTexture");

  // Default program
  glUseProgram(gTextureProgram);
  currentProgram = gTextureProgram;
  changeToProgram = gTextureProgram;

  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ibo);

  glViewport(0, 0, width, height);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

  glVertexAttribPointer(gvtTexCoordHandle, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), gTextureVertices + 3);
  glEnableVertexAttribArray(gvtTexCoordHandle);

  glActiveTexture(GL_TEXTURE0);

  glGenTextures(1, &gTexture);
  glBindTexture(GL_TEXTURE_2D, gTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  glUniform1i(gutTextureHandle, 0);
}

struct RedSquare {
  float x;
  float y;
  float width;
  float height;

  RedSquare(float x_, float y_, float width_, float height_) {
    x = x_;
    y = y_;
    width = width_;
    height = height_;
  }
};

// Fast detector red squares and fast detector red lines
void getRedSquares(std::vector<RedSquare>* redSquares) {
  size_t numSquares = keypoints.size();

  for (size_t i = 0; i < numSquares; i++) {
    float x = keypoints[i].pt.x;
    float y = keypoints[i].pt.y;
    redSquares->push_back(RedSquare(x, y, 0.01f, 0.01f));
  }
}

std::vector<GLfloat> vboData;
std::vector<GLushort> iboData;

void drawSmallRedSquares() {
  std::vector<RedSquare> redSquares;
  getRedSquares(&redSquares);

  size_t numSquares = redSquares.size();
  size_t vboSize = numSquares * 4 * 3 * sizeof(GLfloat);
  size_t iboSize = numSquares * 6 * sizeof(GLushort);

  vboData.resize(vboSize / sizeof(GLfloat));
  iboData.resize(iboSize / sizeof(GLushort));

  int i = 0;
  for (auto redSquare : redSquares) {
    float x = -(redSquare.y / cameraHeight - 0.5f) * 2.0f;
    float y = -(redSquare.x / cameraWidth - 0.5f) * 2.0f;

    size_t squareIndex = i * 4;
    for (size_t j = 0; j < 4; j++) {
      size_t vboIndex = (squareIndex + j) * 3;
      vboData[vboIndex + 0] = redSquares[i].width * gSquareVertices[j * 3 + 0] + x;
      vboData[vboIndex + 1] = redSquares[i].height * gSquareVertices[j * 3 + 1] + y;
      vboData[vboIndex + 2] = 0.0f;
    }

    size_t squareIndex_ibo = i * 6;
    size_t offset_ibo = i * 4;
    for (size_t j = 0; j < 6; j++) {
      iboData[squareIndex_ibo + j] = gIndices[j] + offset_ibo;
    }
    
    i++;
  }

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, vboSize, vboData.data(), GL_DYNAMIC_DRAW);

  // Create and bind the IBO
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

  glBufferData(GL_ELEMENT_ARRAY_BUFFER, iboSize, iboData.data(), GL_DYNAMIC_DRAW);

  // Set up the vertex attribute pointers
  glVertexAttribPointer(gvPositionHandle, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(gvPositionHandle);

  glClear(GL_COLOR_BUFFER_BIT);

  // Draw the squares
  glDrawElements(GL_TRIANGLES, numSquares * 6, GL_UNSIGNED_SHORT, 0);

  glDisableVertexAttribArray(gvPositionHandle);

  glDeleteBuffers(1, &vbo);
  glDeleteBuffers(1, &ibo);
}

void drawSmallRedLines() {
  std::vector<RedSquare> redSquares;
  getRedSquares(&redSquares);
  
  size_t numSquares = redSquares.size();
  size_t vboSize = numSquares * 4 * 3 * sizeof(GLfloat);

  vboData.resize(vboSize / sizeof(GLfloat));

  float width = 0.01f;
  float height = 0.01f;

  int i = 0;
  for (auto redSquare : redSquares) {
    float x = -(redSquare.y / cameraHeight - 0.5f) * 2.0f;
    float y = -(redSquare.x / cameraWidth - 0.5f) * 2.0f;

    size_t squareIndex = i * 4;
    for (size_t j = 0; j < 4; j++) {
      size_t vboIndex = (squareIndex + j) * 3;
      vboData[vboIndex + 0] = width * gSquareVertices[j * 3 + 0] + x;
      vboData[vboIndex + 1] = height * gSquareVertices[j * 3 + 1] + y;
      vboData[vboIndex + 2] = 0.0f;
    }

    i++;
  }

  // VBO
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, vboSize, vboData.data(), GL_DYNAMIC_DRAW);

  // Set up the vertex attribute pointers
  glVertexAttribPointer(gvPositionHandle, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(gvPositionHandle);

  glClear(GL_COLOR_BUFFER_BIT);

  // Draw the lines
  glDrawArrays(GL_TRIANGLES, 0, numSquares * 4);

  glDisableVertexAttribArray(gvPositionHandle);

  glDeleteBuffers(1, &vbo);
}

void drawTexture(unsigned char* imageData, int width, int height) {
  if (imageData == nullptr) {
    return;
  }

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(gIndices), gIndices, GL_STATIC_DRAW);

  glVertexAttribPointer(gvtPositionHandle, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), gTextureVertices);
  glEnableVertexAttribArray(gvtPositionHandle);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, imageData);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

  glDisableVertexAttribArray(gvtPositionHandle);
}

void renderFrame() {
  if (currentProgram != changeToProgram) {
    glUseProgram(changeToProgram);
    currentProgram = changeToProgram;
  }

  if (previewMode == PreviewMode::DETECT_EDGES_FAST) { // Red squares
    drawSmallRedSquares();
  }
  else if (previewMode == PreviewMode::DETECT_EDGES_FAST_LINES) { // Red lines
    drawSmallRedLines();
  }
  else { // Preview edges
    drawTexture(processedImage.data, cameraWidth, cameraHeight);
  }
}

extern "C" {
  JNIEXPORT void JNICALL Java_com_app_edgedetector_MyGLSurfaceView_init(JNIEnv *env, jobject obj,  jint width, jint height);
  JNIEXPORT void JNICALL Java_com_app_edgedetector_MyGLSurfaceView_setCameraSettings(JNIEnv* env, jobject obj, int32_t width, int32_t height);
  JNIEXPORT void JNICALL Java_com_app_edgedetector_MyGLSurfaceView_draw(JNIEnv *env, jobject obj);
  JNIEXPORT void JNICALL Java_com_app_edgedetector_MyGLSurfaceView_processImageBuffers(JNIEnv* env, jobject obj, jobject y, int ySize, int yPixelStride, int yRowStride, jobject u, int uSize, int uPixelStride, int uRowStride, jobject v, int vSize, int vPixelStride, int vRowStride);
  JNIEXPORT void JNICALL Java_com_app_edgedetector_MyGLSurfaceView_touch(JNIEnv *env, jobject obj, int previewMode);
};

JNIEXPORT void JNICALL Java_com_app_edgedetector_MyGLSurfaceView_init(JNIEnv *env, jobject obj,  jint width, jint height) {
  setupDetector();
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

    unsigned char* nv21 = (unsigned char*)malloc(ySize + uSize + vSize);

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
      memcpy(nv21 + yIndex, yData + ySrcIndex, ySizeWidth);
      yIndex += cameraWidth * yPixelStride;

      if (i % 2 == 0) {
        // Use memcpy to copy the U and V data
        memcpy(nv21 + uvIndex, uData + uvSrcIndex, cameraWidth * uPixelStride / 2);
        uvIndex += cameraWidth * uPixelStride / 2;
        memcpy(nv21 + uvIndex, uData + uvSrcIndex, cameraWidth * vPixelStride / 2);
        uvIndex += cameraWidth * vPixelStride / 2;
      }
    }

    grayImage.create(cameraHeight, cameraWidth, CV_8UC1);
    grayImage.data = nv21;
    cv::cvtColor(grayImage, grayImage, cv::COLOR_BGR2RGB);

    // Detection method for image
    detect();

    // Free memory
    free(nv21);

    // Clear image data
    grayImage.release();

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
  previewMode = static_cast<PreviewMode>(previewMode_);
  updatePreviewMode();
}