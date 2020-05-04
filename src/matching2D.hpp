#ifndef matching2D_hpp
#define matching2D_hpp

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdio.h>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <opencv2/xfeatures2d/nonfree.hpp>

#include "dataStructures.h"

void detKeypointsHarris(std::vector<cv::KeyPoint> &keypoints, cv::Mat &img,
                        bool bVis = false);
void detKeypointsShiTomasi(std::vector<cv::KeyPoint> &keypoints, cv::Mat &img,
                           bool bVis = false);
void detKeypointsModern(std::vector<cv::KeyPoint> &keypoints, cv::Mat &img,
                        std::string detectorType, bool bVis = false);
void descKeypoints(std::vector<cv::KeyPoint> &keypoints, cv::Mat &img,
                   cv::Mat &descriptors, std::string descriptorType);
void matchDescriptors(std::vector<cv::KeyPoint> &kPtsSource,
                      std::vector<cv::KeyPoint> &kPtsRef, cv::Mat &descSource,
                      cv::Mat &descRef, std::vector<cv::DMatch> &matches,
                      std::string descriptorType, std::string matcherType,
                      std::string selectorType);

// Stats helpr
class StatsFactory {
private:
    StatsFactory() {}

public:
    static StatsFactory& instance();
    void track(size_t run);
    void updateDet(const std::string& det, double time_ms, const std::vector<cv::KeyPoint> &keypoints);
    void updateDes(const std::string& des, double time_ms);
    void updateMat(const std::vector<cv::DMatch> &matches);
    void write(const std::string& output_path);
private:
    size_t curr_idx;
    std::vector<Stats> storage;
};

#endif /* matching2D_hpp */
