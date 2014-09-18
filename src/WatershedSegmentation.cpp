#include "WatershedSegmentation.hpp"

void WatershedSegmentation::setMarkers (Mat& marker_image) {
    marker_image.convertTo (marker_image, CV_32S);
}

Mat WatershedSegmentation::process(Mat& image, Mat& marker_image) {
    watershed (image, marker_image);
    marker_image.convertTo (marker_image, CV_8U);
    return marker_image;
}

void WatershedSegmentation::segment (std::string in_file, std::string out_file) {

    Mat image = imread (in_file.c_str());
    Mat binary;
    cvtColor(image, binary, CV_RGB2GRAY);
    threshold(binary, binary, 10, 255, THRESH_BINARY);

    // Eliminate noise and smaller objects
    Mat fg;
    erode(binary,fg,Mat(),Point(-1,-1),2);

    // Identify image pixels without objects
    Mat bg;
    dilate(binary,bg,Mat(),Point(-1,-1),3);
    threshold(bg,bg,1, 128,THRESH_BINARY_INV);

    // Create markers image
    Mat markers (binary.size(),CV_8U,Scalar(0));
    markers = fg+bg;

    // Create watershed segmentation object
    setMarkers(markers);

    Mat result = process(image, markers);
    result.convertTo(result,CV_8U);
    imwrite (out_file.c_str(), result);
}
