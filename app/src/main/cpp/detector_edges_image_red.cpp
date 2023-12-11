class Detector_Edges_Image_Red : public Detector_Edges_Image_Color {
public:
  void processColorChannels(std::vector<cv::Mat> &channels, cv::Mat &image) override {
    channels[2] = image; // Only red color
  }
};
