class Detector_Edges_Image_Grayscale : public Detector_Edges_Image {
public:
  void processImage(cv::Mat &image) override {
    // Make edges thicker
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(20, 20));
    cv::dilate(image, image, kernel);

    // Apply pixels from original image to processed image
    currentImage.copyTo(image, image);

    // Update processed image
    processedImage = image;
  }
};
