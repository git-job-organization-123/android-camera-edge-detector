class Renderer_Red_Lines : public Renderer {
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

  void setKeypoints(std::vector<cv::KeyPoint> &keypoints_) override {
    keypoints = keypoints_;
  }

  void draw() override {
    std::vector<GLfloat> vboDataVector;

    GLint numSquares = 0;

    for (const auto& keypoint : keypoints) {
      float x = -(keypoint.pt.y / cameraHeight - 0.5f) * 2.0f;
      float y = -(keypoint.pt.x / cameraWidth - 0.5f) * 2.0f;

      vboDataVector.emplace_back(vertices[0] + x);
      vboDataVector.emplace_back(vertices[1] + y);
      vboDataVector.emplace_back(vertices[2] + x);
      vboDataVector.emplace_back(vertices[3] + y);
      vboDataVector.emplace_back(vertices[4] + x);
      vboDataVector.emplace_back(vertices[5] + y);
      vboDataVector.emplace_back(vertices[6] + x);
      vboDataVector.emplace_back(vertices[7] + y);

      ++numSquares;
    }

    const GLint vboSize = vboDataVector.size() * sizeof(GLfloat);

    // VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vboSize, vboDataVector.data(), GL_DYNAMIC_DRAW);

    // Set up the vertex attribute pointers
    glVertexAttribPointer(positionHandle, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(positionHandle);

    glClear(GL_COLOR_BUFFER_BIT);

    // Draw the lines
    glDrawArrays(GL_TRIANGLES, 0, numSquares * 6);

    glDisableVertexAttribArray(positionHandle);

    glDeleteBuffers(1, &vbo);
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
};
