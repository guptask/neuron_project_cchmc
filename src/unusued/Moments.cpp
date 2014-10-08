#include <vector>
#include "Moments.hpp"

void Moments::apply(cv::Mat src, cv::Mat *dst, unsigned char lower_threshold) {

    // Convert the image to grayscale and blur it
    cv::Mat src_gray;
    cv::blur(src, src_gray, cv::Size(3,3) );

    cv::Mat canny_output;
    std::vector<std::vector<cv::Point> > contours;
    std::vector<cv::Vec4i> hierarchy;

    // Detect edges using canny
    cv::Canny(src_gray, canny_output, lower_threshold, 255, 3);

    // Find contours
    cv::findContours(canny_output, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));

    // Get the moments
    std::vector<cv::Moments> mu(contours.size());
    for (size_t i = 0; i < contours.size(); i++) {
        mu[i] = cv::moments(contours[i], false);
    }

    // Get the mass centers:
    std::vector<cv::Point2f> mc(contours.size());
    for (size_t i = 0; i < contours.size(); i++) {
        mc[i] = cv::Point2f(static_cast<float>(mu[i].m10/mu[i].m00), 
                            static_cast<float>(mu[i].m01/mu[i].m00));
    }

    // Draw contours
    cv::Mat drawing = cv::Mat::zeros(canny_output.size(), CV_8UC3);
    cv::RNG rng(12345);
    for (size_t i = 0; i< contours.size(); i++) {
        cv::Scalar color = cv::Scalar(rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255));
        cv::drawContours(drawing, contours, (int)i, color, 2, 8, hierarchy, 0, cv::Point());
        cv::circle(drawing, mc[i], 4, color, -1, 8, 0);
    }
    *dst = drawing;

    // Calculate the area with the moments 00 and compare with the result of the OpenCV function
    /*for( size_t i = 0; i< contours.size(); i++ ) {
        printf(" * Contour[%d] - Area (M_00) = %.2f - Area OpenCV: %.2f - Length: %.2f \n", (int)i, mu[i].m00, contourArea(contours[i]), arcLength( contours[i], true ) );
        Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
        drawContours( drawing, contours, (int)i, color, 2, 8, hierarchy, 0, Point() );
        circle( drawing, mc[i], 4, color, -1, 8, 0 );
    }*/
}
