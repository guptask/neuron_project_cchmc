#include "EqualizeHistogram.hpp"

void EqualizeHistogram::apply (std::string in_file, std::string out_file) {

    Mat src = imread (in_file.c_str());
    if (src.empty()) return;

    // Create a matrix of the same type and size as src (for dst)
    Mat dst;
    dst.create(src.size(), src.type());

    // Convert the image to grayscale
    cvtColor (src, src, COLOR_RGB2GRAY);

    /// Apply Histogram Equalization
    equalizeHist (src, dst);

    imwrite (out_file.c_str(), dst);
}
