class Renderer_Red_Squares : public Renderer {
public:
  const char* getVertexShader() override {
    return R"(#version 300 es
      layout(location = 0) in vec2 vPosition;

      void main() {
        gl_Position = vec4(vPosition, 0.0, 1.0);
      }
    )";
  }
  
  const char* getFragmentShader() override {
    return R"(#version 300 es
      precision mediump float;
      out vec4 fragColor;

      void main() {
        fragColor = vec4(1.0, 0.0, 0.0, 1.0); // Red
      }
    )";
  }

  void setupProgram() override {
    program = createProgram(getVertexShader(), getFragmentShader());
    if (!program) {
      return;
    }

    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ibo);

    positionHandle = glGetAttribLocation(program, "vPosition");
  }

  void init() override {
    vboData = new GLfloat[8192 * 8 * sizeof(GLfloat)];
    iboData = new GLushort[8192 * 6 * sizeof(GLushort)];
  }

  void setKeypoints(std::vector<cv::KeyPoint> &keypoints_) override {
    keypoints = keypoints_;
  }

  void draw() override {
    GLint numSquares = 0;

    for (const auto& keypoint : keypoints) {
      if (numSquares >= 8192) {
        // Prevent crash with limit
        continue;
      }

      float x = -(keypoint.pt.y / cameraHeight - 0.5f) * 2.0f;
      float y = -(keypoint.pt.x / cameraWidth - 0.5f) * 2.0f;

      const GLint vboPosition = numSquares * 8;
      vboData[vboPosition] =     vertices[0] + x;
      vboData[vboPosition + 1] = vertices[1] + y;
      vboData[vboPosition + 2] = vertices[2] + x;
      vboData[vboPosition + 3] = vertices[3] + y;
      vboData[vboPosition + 4] = vertices[4] + x;
      vboData[vboPosition + 5] = vertices[5] + y;
      vboData[vboPosition + 6] = vertices[6] + x;
      vboData[vboPosition + 7] = vertices[7] + y;

      const GLint iboPosition = numSquares * 6;
      const GLint iboOffset = numSquares * 4;
      iboData[iboPosition] =     indices[0] + iboOffset;
      iboData[iboPosition + 1] = indices[1] + iboOffset;
      iboData[iboPosition + 2] = indices[2] + iboOffset;
      iboData[iboPosition + 3] = indices[3] + iboOffset;
      iboData[iboPosition + 4] = indices[4] + iboOffset;
      iboData[iboPosition + 5] = indices[5] + iboOffset;

      ++numSquares;
    }

    const GLint vboSize = numSquares * 8 * sizeof(GLfloat);
    const GLint iboSize = numSquares * 6 * sizeof(GLushort);

    // VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vboSize, vboData, GL_DYNAMIC_DRAW);

    // IBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, iboSize, iboData, GL_DYNAMIC_DRAW);

    // Set up the vertex attribute pointers
    glVertexAttribPointer(positionHandle, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(positionHandle);

    glClear(GL_COLOR_BUFFER_BIT);

    // Draw the squares
    glDrawElements(GL_TRIANGLES, numSquares * 6, GL_UNSIGNED_SHORT, 0);

    glDisableVertexAttribArray(positionHandle);

    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ibo);
  }

  void clear() override {
    delete[] vboData;
    delete[] iboData;
  }

private:
  GLuint positionHandle;

  std::vector<cv::KeyPoint> keypoints;

  // Square
  const GLfloat vertices[8] = {
     0.005f ,  0.005f , // top right
     0.005f , -0.005f , // bottom right
    -0.005f , -0.005f , // bottom left
    -0.005f ,  0.005f   // top left
  };

  const GLushort indices[6] = {
    0, 1, 2,
    2, 3, 0
  };

  GLfloat *vboData;
  GLushort *iboData;
};
