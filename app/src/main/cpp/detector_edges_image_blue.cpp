class Detector_Edges_Image_Blue : public Detector_Edges_Image_Color {
public:
  void processColorChannels(std::vector<cv::Mat> &channels, cv::Mat &image) override {
    channels[0] = image; // Only blue color
  }
};
