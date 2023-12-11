class Detector_Edges_Image_Color : public Detector_Edges_Image {
public:
  void processImage(cv::Mat &image) override {
    // Make edges thicker
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(1, 1));
    cv::dilate(image, image, kernel);

    // Process image colors
    processColors(image);

    // Update processed image
    processedImage = image;
  }

  virtual void processColors(cv::Mat &image) {
    cv::Mat coloredImage;

    // Convert image to BGR
    cv::cvtColor(image, coloredImage, cv::COLOR_GRAY2BGR);

    std::vector<cv::Mat> channels;

    // Get color channels
    cv::split(coloredImage, channels);

    // Set all channels to 0
    channels[0] = channels[1] = channels[2] = 0;

    // Process image color channels
    processColorChannels(channels, image);
    
    // Merge colors channels to image
    cv::merge(channels, image);
  }

  virtual void processColorChannels(std::vector<cv::Mat> &channels, cv::Mat &Image) {}
};
