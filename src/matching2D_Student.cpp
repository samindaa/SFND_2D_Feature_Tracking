#include "matching2D.hpp"
#include <fstream>
#include <numeric>

using namespace std;

// Find best matches for keypoints in two camera images based on several
// matching methods
void matchDescriptors(std::vector<cv::KeyPoint> &kPtsSource,
                      std::vector<cv::KeyPoint> &kPtsRef, cv::Mat &descSource,
                      cv::Mat &descRef, std::vector<cv::DMatch> &matches,
                      std::string descriptorType, std::string matcherType,
                      std::string selectorType) {
  // configure matcher
  bool crossCheck = false;
  cv::Ptr<cv::DescriptorMatcher> matcher;

  std::cout << "descriptorType: " << descriptorType
            << " matcherType: " << matcherType
            << " selectorType: " << selectorType << std::endl;
  // MAT_BF/MAT_FLANN
  if (matcherType.compare("MAT_BF") == 0) {
    // DES_BINARY/DES_HOG
    int normType = descriptorType.compare("DES_BINARY") == 0 ? cv::NORM_HAMMING
                                                             : cv::NORM_L2;
    matcher = cv::BFMatcher::create(normType, crossCheck);
  } else if (matcherType.compare("MAT_FLANN") == 0) {
    // From class notes: OpenCV bug workaround: convert binary descriptors to
    // floating point due to bug in current OpenCV implementation.
    if (descSource.type() != CV_32F) {
      descSource.convertTo(descSource, CV_32F);
    }
    if (descRef.type() != CV_32F) {
      descRef.convertTo(descRef, CV_32F);
    }
    matcher = cv::DescriptorMatcher::create(cv::DescriptorMatcher::FLANNBASED);
  } else {
    throw std::logic_error("matcherType: " + matcherType +
                           " is not supported.");
  }

  // perform matching task
  if (selectorType.compare("SEL_NN") == 0) { // nearest neighbor (best match)
    double t = (double)cv::getTickCount();
    matcher->match(
        descSource, descRef,
        matches); // Finds the best match for each descriptor in desc1
    t = ((double)cv::getTickCount() - t) / cv::getTickFrequency();
    std::cout << "(NN) with n= " << matches.size() << " matches in "
              << 1000 * t / 1.0 << " ms" << std::endl;
  } else if (selectorType.compare("SEL_KNN") ==
             0) { // k nearest neighbors (k=2)
    std::vector<std::vector<cv::DMatch>> knn_matches;
    double t = (double)cv::getTickCount();
    matcher->knnMatch(descSource, descRef, knn_matches,
                      2); // find the 2 best matches
    std::cout << "(kNN) with n= " << knn_matches.size() << " knn_matches in "
              << 1000 * t / 1.0 << " ms" << std::endl;

    const double minDescDistRatio = 0.8;
    for (auto it = knn_matches.begin(); it != knn_matches.end(); ++it) {

      if ((*it)[0].distance < minDescDistRatio * (*it)[1].distance) {
        matches.push_back((*it)[0]);
      }
    }
    std::cout << "# keypoints removed = " << knn_matches.size() - matches.size()
              << std::endl;
    StatsFactory::instance().updateMat(matches);
  } else {
    throw std::logic_error("selectorType: " + selectorType +
                           " is not supported.");
  }
}

// Use one of several types of state-of-art descriptors to uniquely identify
// keypoints
void descKeypoints(vector<cv::KeyPoint> &keypoints, cv::Mat &img,
                   cv::Mat &descriptors, string descriptorType) {
  // select appropriate descriptor
  cv::Ptr<cv::DescriptorExtractor> extractor;
  if (descriptorType.compare("BRISK") == 0) {

    int threshold = 30;        // FAST/AGAST detection threshold score.
    int octaves = 3;           // detection octaves (use 0 to do single scale)
    float patternScale = 1.0f; // apply this scale to the pattern used for
                               // sampling the neighbourhood of a keypoint.

    extractor = cv::BRISK::create(threshold, octaves, patternScale);
  } else if (descriptorType.compare("BRIEF")) {
    int bytes = 32; // legth of the descriptor in bytes, valid values are: 16,
                    // 32 (default) or 64 .
    extractor = cv::xfeatures2d::BriefDescriptorExtractor::create(bytes);
  } else if (descriptorType.compare("ORB")) {
    extractor = cv::ORB::create();
  } else if (descriptorType.compare("FREAK")) {
    extractor = cv::xfeatures2d::FREAK::create();
  } else if (descriptorType.compare("AKAZE")) {
    extractor = cv::AKAZE::create();
  } else if (descriptorType.compare("SIFT")) {
    extractor = cv::xfeatures2d::SIFT::create();
  } else {
    throw std::logic_error("descriptorType: " + descriptorType +
                           " is not supported.");
  }

  // perform feature description
  double t = (double)cv::getTickCount();
  extractor->compute(img, keypoints, descriptors);
  t = ((double)cv::getTickCount() - t) / cv::getTickFrequency();
  cout << descriptorType << " descriptor extraction in " << 1000 * t / 1.0
       << " ms" << endl;
  StatsFactory::instance().updateDes(descriptorType, t);
}

// Detect keypoints in image using the traditional Shi-Thomasi detector
void detKeypointsShiTomasi(vector<cv::KeyPoint> &keypoints, cv::Mat &img,
                           bool bVis) {
  // compute detector parameters based on image size
  int blockSize = 4; //  size of an average block for computing a derivative
                     //  covariation matrix over each pixel neighborhood
  double maxOverlap = 0.0; // max. permissible overlap between two features in %
  double minDistance = (1.0 - maxOverlap) * blockSize;
  int maxCorners =
      img.rows * img.cols / max(1.0, minDistance); // max. num. of keypoints

  double qualityLevel = 0.01; // minimal accepted quality of image corners
  double k = 0.04;

  // Apply corner detection
  double t = (double)cv::getTickCount();
  vector<cv::Point2f> corners;
  cv::goodFeaturesToTrack(img, corners, maxCorners, qualityLevel, minDistance,
                          cv::Mat(), blockSize, false, k);

  // add corners to result vector
  for (auto it = corners.begin(); it != corners.end(); ++it) {

    cv::KeyPoint newKeyPoint;
    newKeyPoint.pt = cv::Point2f((*it).x, (*it).y);
    newKeyPoint.size = blockSize;
    keypoints.push_back(newKeyPoint);
  }
  t = ((double)cv::getTickCount() - t) / cv::getTickFrequency();
  cout << "Shi-Tomasi detection with n=" << keypoints.size() << " keypoints in "
       << 1000 * t / 1.0 << " ms" << endl;
  StatsFactory::instance().updateDet("SHITOMASI", t, keypoints);

  // visualize results
  if (bVis) {
    cv::Mat visImage = img.clone();
    cv::drawKeypoints(img, keypoints, visImage, cv::Scalar::all(-1),
                      cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
    string windowName = "Shi-Tomasi Corner Detector Results";
    cv::namedWindow(windowName, 6);
    cv::imshow(windowName, visImage);
    cv::waitKey(0);
  }
}

// Detect keypoints in image using the traditional
void detKeypointsHarris(std::vector<cv::KeyPoint> &keypoints, cv::Mat &img,
                        bool bVis) {
  // Detector parameters
  int blockSize =
      2; // for every pixel, a blockSize × blockSize neighborhood is considered
  int apertureSize = 3; // aperture parameter for Sobel operator (must be odd)
  int minResponse =
      100; // minimum value for a corner in the 8bit scaled response matrix
  double k = 0.04; // Harris parameter (see equation for details)

  // Detect Harris corners and normalize output
  double t = (double)cv::getTickCount();
  cv::Mat dst, dst_norm, dst_norm_scaled;
  dst = cv::Mat::zeros(img.size(), CV_32FC1);
  cv::cornerHarris(img, dst, blockSize, apertureSize, k, cv::BORDER_DEFAULT);
  cv::normalize(dst, dst_norm, 0, 255, cv::NORM_MINMAX, CV_32FC1, cv::Mat());
  cv::convertScaleAbs(dst_norm, dst_norm_scaled);

  // Look for prominent corners and instantiate keypoints
  double maxOverlap = 0.0; // max. permissible overlap between two features in
                           // %, used during non-maxima suppression
  for (size_t j = 0; j < dst_norm.rows; j++) {
    for (size_t i = 0; i < dst_norm.cols; i++) {
      int response = (int)dst_norm.at<float>(j, i);
      if (response > minResponse) { // only store points above a threshold

        cv::KeyPoint newKeyPoint;
        newKeyPoint.pt = cv::Point2f(i, j);
        newKeyPoint.size = 2 * apertureSize;
        newKeyPoint.response = response;

        // perform non-maximum suppression (NMS) in local neighbourhood around
        // new key point
        bool bOverlap = false;
        for (auto it = keypoints.begin(); it != keypoints.end(); ++it) {
          double kptOverlap = cv::KeyPoint::overlap(newKeyPoint, *it);
          if (kptOverlap > maxOverlap) {
            bOverlap = true;
            if (newKeyPoint.response >
                (*it).response) { // if overlap is >t AND response is higher for
                                  // new kpt
              *it = newKeyPoint;  // replace old key point with new one
              break;              // quit loop over keypoints
            }
          }
        }
        if (!bOverlap) { // only add new key point if no overlap has been found
                         // in previous NMS
          keypoints.push_back(
              newKeyPoint); // store new keypoint in dynamic list
        }
      }
    } // eof loop over cols
  }   // eof loop over rows

  t = ((double)cv::getTickCount() - t) / cv::getTickFrequency();
  cout << "Harris detection with n=" << keypoints.size() << " keypoints in "
       << 1000 * t / 1.0 << " ms" << endl;
  StatsFactory::instance().updateDet("HARRIS", t, keypoints);

  if (bVis) {
    // visualize keypoints
    std::string windowName = "Harris Corner Detection Results";
    cv::namedWindow(windowName, 5);
    cv::Mat visImage = dst_norm_scaled.clone();
    cv::drawKeypoints(dst_norm_scaled, keypoints, visImage, cv::Scalar::all(-1),
                      cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
    cv::imshow(windowName, visImage);
    cv::waitKey(0);
  }
}

void detKeypointsModern(std::vector<cv::KeyPoint> &keypoints, cv::Mat &img,
                        std::string detectorType, bool bVis) {
  cv::Ptr<cv::FeatureDetector> detector = nullptr;
  if (detectorType.compare("FAST") == 0) {
    int threshold = 30; // difference between intensity of the central pixel and
                        // pixels of a circle around this pixel
    bool bNMS = true;   // perform non-maxima suppression on keypoints
    cv::FastFeatureDetector::DetectorType type =
        cv::FastFeatureDetector::TYPE_9_16; // TYPE_9_16, TYPE_7_12, TYPE_5_8
    detector = cv::FastFeatureDetector::create(threshold, bNMS, type);
  } else if (detectorType.compare("BRISK") == 0) {
    detector = cv::BRISK::create();
  } else if (detectorType.compare("ORB") == 0) {
    detector = cv::ORB::create();
  } else if (detectorType.compare("AKAZE") == 0) {
    detector = cv::AKAZE::create();
  } else if (detectorType.compare("FREAK") == 0) {
    detector = cv::xfeatures2d::FREAK::create();
  } else if (detectorType.compare("SIFT") == 0) {
    detector = cv::xfeatures2d::SIFT::create();
  } else {
    throw std::logic_error("detectorType: " + detectorType +
                           " is not supported.");
  }

  double t = (double)cv::getTickCount();
  detector->detect(img, keypoints);
  t = ((double)cv::getTickCount() - t) / cv::getTickFrequency();
  cout << detectorType << " with n= " << keypoints.size() << " keypoints in "
       << 1000 * t / 1.0 << " ms" << endl;
  StatsFactory::instance().updateDet(detectorType, t, keypoints);

  if (bVis) {
    // visualize keypoints
    std::string windowName = detectorType + " Detection Results";
    cv::Mat visImage = img.clone();
    cv::drawKeypoints(img, keypoints, visImage, cv::Scalar::all(-1),
                      cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
    cv::imshow(windowName, visImage);
    cv::waitKey(0);
  }
}

// Helpers to track the stats
StatsFactory &StatsFactory::instance() {
  static StatsFactory instance;
  return instance;
}
void StatsFactory::track(size_t run) {
  curr_idx = storage.size();
  storage.emplace_back(Stats());
  storage[curr_idx].run = run;
}
void StatsFactory::updateDet(const std::string &det, double time_ms,
                             const std::vector<cv::KeyPoint> &keypoints) {
  auto &stats = storage[curr_idx];
  stats.det = det;
  stats.keypoints = keypoints.size();
  stats.det_time_ms = time_ms;
  double size_mu =
      std::accumulate(keypoints.begin(), keypoints.end(), 0.0,
                      [](const double &sum, const cv::KeyPoint &kp) {
                        return sum + kp.size;
                      }) /
      std::max(keypoints.size(),
               static_cast<std::vector<cv::KeyPoint>::size_type>(1));
  stats.size_std =
      std::accumulate(keypoints.begin(), keypoints.end(), 0.0,
                      [&size_mu](const double &sum, const cv::KeyPoint &kp) {
                        double d = kp.size - size_mu;
                        return sum + d * d;
                      }) /
      std::max(keypoints.size(),
               static_cast<std::vector<cv::KeyPoint>::size_type>(1));
  stats.size_mu = size_mu;
}
void StatsFactory::updateDes(const std::string &des, double time_ms) {
  auto &stats = storage[curr_idx];
  stats.des = des;
  stats.des_time_ms = time_ms;
}
void StatsFactory::updateMat(const std::vector<cv::DMatch> &matches) {
  auto &stats = storage[curr_idx];
  stats.matches = matches.size();
}
void StatsFactory::write(const std::string &output_path) {
  std::ofstream out;
  out.open(output_path);
  std::stringstream head_ss;
  head_ss << "run,det,des,keypoints,det_time_ms,des_time_ms,size_mu,size_std,"
             "matches";
  out << head_ss.str() << "\n";
  for (auto iter = storage.begin(); iter != storage.end(); ++iter) {
    std::stringstream ss;
    ss << iter->run << "," << iter->det << "," << iter->des << ","
       << iter->keypoints << "," << iter->det_time_ms << ","
       << iter->des_time_ms << "," << iter->size_mu << "," << iter->size_std
       << "," << iter->matches;
    //    std::cout << "output: " << ss.str() << std::endl;
    out << ss.str() << "\n";
  }
  out.close();
}
