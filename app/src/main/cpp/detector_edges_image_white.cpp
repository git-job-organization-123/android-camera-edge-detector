class Detector_Edges_Image_White : public Detector_Edges_Image {
  void processImage(cv::Mat &image) override {
    // Update processed image
    processedImage = image;
  }
};
