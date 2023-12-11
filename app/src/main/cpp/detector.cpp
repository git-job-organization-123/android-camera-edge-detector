class Detector {
public:
  cv::Mat currentImage; // NV21 image from Android device
  cv::Mat processedImage; // Detector processed image

  std::vector<cv::KeyPoint> keypoints; // Detected points

  virtual void init() {}

  virtual void setImageData(unsigned char* nv21ImageData) {
    // Create image
    currentImage.create(cameraHeight, cameraWidth, CV_8UC1);

    // Use NV21 image data
    currentImage.data = nv21ImageData;

    // Convert image to grayscale
    cv::cvtColor(currentImage, currentImage, cv::COLOR_BGR2RGB);
  }

  virtual void detect() {}

  virtual void clear() {
    clearImage();
    clearProcessedImage();
  }

  void clearImage() {
    currentImage.release();
  }

  void clearProcessedImage() {
    processedImage.release();
  }
};
