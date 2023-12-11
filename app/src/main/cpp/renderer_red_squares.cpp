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
    int i = 0;

    std::for_each(keypoints.begin(), keypoints.end(), [i, this](cv::KeyPoint keypoint) mutable {
      float x = -(keypoint.pt.y / cameraHeight - 0.5f) * 2.0f;
      float y = -(keypoint.pt.x / cameraWidth - 0.5f) * 2.0f;

      const uint32_t vboIndex = i * 8;
      vboData[vboIndex + 0] = squareVertices[0] + x;
      vboData[vboIndex + 1] = squareVertices[1] + y;
      vboData[vboIndex + 2] = squareVertices[2] + x;
      vboData[vboIndex + 3] = squareVertices[3] + y;
      vboData[vboIndex + 4] = squareVertices[4] + x;
      vboData[vboIndex + 5] = squareVertices[5] + y;
      vboData[vboIndex + 6] = squareVertices[6] + x;
      vboData[vboIndex + 7] = squareVertices[7] + y;

      const uint32_t iboIndex = i * 6;
      const uint32_t offsetIbo = i * 4;
      iboData[iboIndex + 0] = indices[0] + offsetIbo;
      iboData[iboIndex + 1] = indices[1] + offsetIbo;
      iboData[iboIndex + 2] = indices[2] + offsetIbo;
      iboData[iboIndex + 3] = indices[3] + offsetIbo;
      iboData[iboIndex + 4] = indices[4] + offsetIbo;
      iboData[iboIndex + 5] = indices[5] + offsetIbo;

      ++i;
    });

    size_t numSquares = keypoints.size();
    size_t vboSize = numSquares * 32;
    size_t iboSize = numSquares * 24;

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

private:
  std::vector<cv::KeyPoint> keypoints;
};
