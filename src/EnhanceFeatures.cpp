#include "EnhanceFeatures.hpp"

void EnhanceFeatures::apply (std::string in_file, std::string out_file) {

    Mat src = imread (in_file.c_str());
    if (src.empty()) return;

    // Create a matrix of the same type and size as src (for dst)
    Mat dst;
    dst.create(src.size(), src.type());

    // Convert the image to grayscale
    cvtColor (src, src, COLOR_RGB2GRAY);

    // Apply Gaussian blur and Otsu threshold
    GaussianBlur (src, dst, Size(5,5), 0, 0);
    threshold (src, dst, 200, 255, THRESH_BINARY+THRESH_OTSU+THRESH_TRUNC);

    imwrite (out_file.c_str(), dst);
}
