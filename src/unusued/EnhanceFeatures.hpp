#ifndef ENHANCE_FEATURES_HPP
#define ENHANCE_FEATURES_HPP

/* Enhance Features
 */

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/features2d/features2d.hpp"

using namespace cv;

class EnhanceFeatures {

public:
    EnhanceFeatures() = default;

    void apply (std::string in_file, std::string out_file);
};

#endif
