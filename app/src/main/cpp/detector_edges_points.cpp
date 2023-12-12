class Detector_Edges_Points : public Detector_Edges {
public:
  void init() override {
    cv::Ptr<cv::FastFeatureDetector> fast = cv::FastFeatureDetector::create();
    fast->setThreshold(12); // 10 = default
    featureDetector = fast;
  }

  void detect() override {
    // Create a list to hold the keypoints
    featureDetector->detect(currentImage, keypoints);
  }

  void updateRendererData(Renderer *renderer) override {
    // Update renderer keypoints
    renderer->setKeypoints(keypoints);
  }

private:
  cv::Ptr<cv::Feature2D> featureDetector;
};
