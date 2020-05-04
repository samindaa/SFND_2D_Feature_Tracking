#ifndef dataStructures_h
#define dataStructures_h

#include <opencv2/core.hpp>
#include <vector>

struct DataFrame { // represents the available sensor information at the same
                   // time instance

  cv::Mat cameraImg; // camera image

  std::vector<cv::KeyPoint> keypoints; // 2D keypoints within camera image
  cv::Mat descriptors;                 // keypoint descriptors
  std::vector<cv::DMatch>
      kptMatches; // keypoint matches between previous and current frame
};

// Helper to collect data
struct Stats {
  std::string det;
  std::string des;
  size_t run;
  size_t keypoints;
  double det_time_ms;
  double des_time_ms;
  double size_mu;
  double size_std;
  size_t matches;

  Stats()
      : run(0), keypoints(0), det_time_ms(0), des_time_ms(0), size_mu(0),
        size_std(0), matches(0) {}
};

#endif /* dataStructures_h */
