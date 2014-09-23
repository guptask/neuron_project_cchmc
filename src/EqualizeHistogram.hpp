#ifndef EQUALIZE_HISTOGRAM_HPP
#define EQUALIZE_HISTOGRAM_HPP

/* Equalize Histogram
 */

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace cv;

class EqualizeHistogram {

public:
    EqualizeHistogram() = default;

    void apply (std::string in_file, std::string out_file);
};

#endif
