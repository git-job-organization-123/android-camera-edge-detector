class Renderer_Red_Squares : public Renderer {
public:
  const float squareWidth = 0.01f;
  const float squareHeight = 0.01f;

  // Square
  const GLfloat squareVertices[8] = {
     0.5f * squareWidth,  0.5f * squareHeight, // top right
     0.5f * squareWidth, -0.5f * squareHeight, // bottom right
    -0.5f * squareWidth, -0.5f * squareHeight, // bottom left
    -0.5f * squareWidth,  0.5f * squareHeight  // top left
  };

  Renderer_Red_Squares(GLuint program_)
  : Renderer(program_) {
  }

  void setKeypoints(std::vector<cv::KeyPoint> &keypoints_) override {
    keypoints = keypoints_;
  }

  void draw() override {
    std::vector<GLfloat> vboDataVector;
    std::vector<GLushort> iboDataVector;

    GLint numSquares = 0;

    for (const auto& keypoint : keypoints) {
      float x = -(keypoint.pt.y / cameraHeight - 0.5f) * 2.0f;
      float y = -(keypoint.pt.x / cameraWidth - 0.5f) * 2.0f;

      vboDataVector.emplace_back(squareVertices[0] + x);
      vboDataVector.emplace_back(squareVertices[1] + y);
      vboDataVector.emplace_back(squareVertices[2] + x);
      vboDataVector.emplace_back(squareVertices[3] + y);
      vboDataVector.emplace_back(squareVertices[4] + x);
      vboDataVector.emplace_back(squareVertices[5] + y);
      vboDataVector.emplace_back(squareVertices[6] + x);
      vboDataVector.emplace_back(squareVertices[7] + y);

      const GLint offsetIbo = numSquares * 4;
      iboDataVector.emplace_back(indices[0] + offsetIbo);
      iboDataVector.emplace_back(indices[1] + offsetIbo);
      iboDataVector.emplace_back(indices[2] + offsetIbo);
      iboDataVector.emplace_back(indices[3] + offsetIbo);
      iboDataVector.emplace_back(indices[4] + offsetIbo);
      iboDataVector.emplace_back(indices[5] + offsetIbo);

      ++numSquares;
    }

    const GLint vboSize = vboDataVector.size() * sizeof(GLfloat);
    const GLint iboSize = iboDataVector.size() * sizeof(GLushort);

    // VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vboSize, vboDataVector.data(), GL_DYNAMIC_DRAW);

    // IBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, iboSize, iboDataVector.data(), GL_DYNAMIC_DRAW);

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

private:
  std::vector<cv::KeyPoint> keypoints;
};
