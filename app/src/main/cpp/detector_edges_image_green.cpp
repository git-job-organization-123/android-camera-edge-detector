class Detector_Edges_Image_Green : public Detector_Edges_Image_Color {
public:
  void processColorChannels(std::vector<cv::Mat> &channels, cv::Mat &image) override {
    channels[1] = image; // Only green color
  }
};
