class Detector_Edges_Image : public Detector_Edges {
public:
  void detect() override {
    cv::Mat image;

    // Detect edges from current image and add them to blank image
    cv::Canny(currentImage, image, 80, 90);

    // Process image
    processImage(image);

    // Convert processed image to RGB
    cv::cvtColor(processedImage, processedImage, cv::COLOR_BGR2RGB);
  }

  virtual void processImage(cv::Mat &image) {}
};
