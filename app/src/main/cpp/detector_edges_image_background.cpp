class Detector_Edges_Image_Background : public Detector_Edges_Image {
public:
  void processImage(cv::Mat &image) override {
    // Convert image to RGB
    cv::cvtColor(image, image, cv::COLOR_BGR2RGB);

    // Copy edges to current image
    cv::addWeighted(image, 0.5, currentImage, 0.5, 0, image);

    // Update processed image
    processedImage = image;
  }
};
