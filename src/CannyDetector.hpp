#ifndef CANNY_DETECTOR_HPP
#define CANNY_DETECTOR_HPP

/* Canny edge detector
 */

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace cv;

class CannyDetector {

public:
    CannyDetector() = default;

    void detectEdges (std::string in_file, std::string out_file);
};

#endif
