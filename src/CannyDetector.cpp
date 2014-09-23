#include "CannyDetector.hpp"

void CannyDetector::apply (std::string in_file, std::string out_file) {

    Mat src = imread (in_file.c_str());
    if (src.empty()) return;

    // Create a matrix of the same type and size as src (for dst)
    Mat dst;
    dst.create(src.size(), src.type());

    // Convert the image to grayscale
    Mat src_gray;
    cvtColor (src, src_gray, COLOR_BGR2GRAY);

    // Reduce noise with a kernel 3x3
    Mat detected_edges;
    blur (src_gray, detected_edges, Size(3,3));

    /// Canny detector
    int low_threshold = 0;
    Canny (detected_edges, detected_edges, low_threshold, low_threshold*3, 3);

    dst = Scalar::all(0);
    src.copyTo (dst, detected_edges);
    imwrite (out_file.c_str(), dst);
}
