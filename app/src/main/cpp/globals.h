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

GLfloat *vboData;
GLushort *iboData;

int cameraWidth;
int cameraHeight;
