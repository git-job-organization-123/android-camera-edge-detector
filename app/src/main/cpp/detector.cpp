#include <opencv2/core.hpp> // OpenCV core
#include <opencv2/imgproc.hpp> // OpenCV COLOR_
#include <opencv2/features2d.hpp> // OpenCV fast feature detector

using namespace cv;

// NV21 image from Android device
cv::Mat grayImage;

// Edge image
cv::Mat processedImage;

// Fast feature detector
cv::Ptr<cv::Feature2D> featureDetector;

// Detected points (red squares)
std::vector<cv::KeyPoint> keypoints;

void updateDetector() {
  if (previewMode == PreviewMode::DETECT_EDGES_FAST 
   || previewMode == PreviewMode::DETECT_EDGES_FAST_LINES) { // Red squares and red lines
    cv::Ptr<cv::FastFeatureDetector> fast = cv::FastFeatureDetector::create();
    fast->setThreshold(12); // 10 = default
    featureDetector = fast;
  }
}

void setupDetector() {
  updateDetector();
}

void detectKeypoints() {
  // Create a list to hold the keypoints
  featureDetector->detect(grayImage, keypoints);
}

void dilate(cv::Mat* image, int x, int y) {
  cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5,5));
  cv::dilate(*image, *image, kernel);
}

void colorEdges(cv::Mat* edgeImage) {
  cv::Mat colorEdges;
  cv::cvtColor(*edgeImage, colorEdges, cv::COLOR_GRAY2BGR);
  std::vector<cv::Mat> channels;
  cv::split(colorEdges, channels);
  channels[0] = channels[1] = channels[2] = 0;

  if (previewMode == PreviewMode::DETECT_PREVIEW_EDGES_RED) {
    channels[2] = *edgeImage;
  }
  else if (previewMode == PreviewMode::DETECT_PREVIEW_EDGES_GREEN) {
    channels[1] = *edgeImage;
  }
  else if (previewMode == PreviewMode::DETECT_PREVIEW_EDGES_BLUE) {
    channels[0] = *edgeImage;
  }

  cv::merge(channels, colorEdges);

  *edgeImage = colorEdges;
}

void detectEdges(cv::Mat* edgeImage) {
  cv::Canny(grayImage, *edgeImage, 80, 90);

  if (previewMode == PreviewMode::DETECT_PREVIEW_EDGES_RED
   || previewMode == PreviewMode::DETECT_PREVIEW_EDGES_GREEN
   || previewMode == PreviewMode::DETECT_PREVIEW_EDGES_BLUE) {
    // Make edges thicker
    dilate(edgeImage, 1, 1);

    colorEdges(edgeImage);
  }
  else if (previewMode == PreviewMode::DETECT_PREVIEW_EDGES_GRAYSCALE) {
    // Make edges thicker
    dilate(edgeImage, 20, 20);

    // Apply pixels from original image to edge image
    grayImage.copyTo(*edgeImage, *edgeImage);
  }
  else if (previewMode == PreviewMode::DETECT_PREVIEW_EDGES_WHITE_WITH_BACKGROUND) {
    cv::cvtColor(*edgeImage, *edgeImage, cv::COLOR_BGR2RGB);

    // Copy edges to preview image
    cv::addWeighted(*edgeImage, 0.5, grayImage, 0.5, 0, *edgeImage);
  }
}

void detect() {
  if (previewMode == PreviewMode::DETECT_EDGES_FAST
   || previewMode == PreviewMode::DETECT_EDGES_FAST_LINES) {  // Fast detector red squares and fast detector red lines
    detectKeypoints();
  }
  else if (previewMode == PreviewMode::DETECT_PREVIEW_EDGES_WHITE
        || previewMode == PreviewMode::DETECT_PREVIEW_EDGES_RED
        || previewMode == PreviewMode::DETECT_PREVIEW_EDGES_GREEN
        || previewMode == PreviewMode::DETECT_PREVIEW_EDGES_BLUE
        || previewMode == PreviewMode::DETECT_PREVIEW_EDGES_GRAYSCALE
        || previewMode == PreviewMode::DETECT_PREVIEW_EDGES_WHITE_WITH_BACKGROUND) { // Preview edges
    detectEdges(&processedImage);
    cv::cvtColor(processedImage, processedImage, cv::COLOR_BGR2RGB);
  }
}
