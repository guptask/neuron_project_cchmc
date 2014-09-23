#ifndef WATERSHED_SEGMENTATION_HPP
#define WATERSHED_SEGMENTATION_HPP

/* Watershed segmentation algorithm
 */

#include "opencv/cv.h"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace cv;

class WatershedSegmentation {

public:
    WatershedSegmentation() = default;

    void apply (std::string in_file, std::string out_file);


private:
    void setMarkers (Mat& marker_image);

    Mat process (Mat& image, Mat& marker_image);
};

#endif
