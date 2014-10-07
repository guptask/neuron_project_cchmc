#ifndef MOMENTS_HPP
#define MOMENTS_HPP

/* Moments calculation
 */

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"

class Moments {
public:
    Moments() = default;

    void apply(cv::Mat src, cv::Mat *dst, unsigned char lower_threshold);
};

#endif
