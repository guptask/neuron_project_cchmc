#include <iostream>
#include <dirent.h>
#include <sys/stat.h>

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"

#define NUM_Z_LAYERS  3  // Merge a certain number of z layers

/* Channel type */
enum class ChannelType : unsigned char {
    BLUE = 0,
    GREEN,
    RED
};

/* Enhance the image */
bool enhanceImage(cv::Mat src, ChannelType channel_type, cv::Mat *dst) {

    // Convert to grayscale and equalize the histogram
    cv::Mat src_equalized;
    cvtColor (src, src_equalized, cv::COLOR_BGR2GRAY);
    equalizeHist(src_equalized, src_equalized);

    // Enhance the image using Gaussian blur and thresholding
    cv::Mat enhanced;
    switch(channel_type) {
        case ChannelType::BLUE: {
            cv::GaussianBlur(src_equalized, enhanced, cv::Size(5,5), 0, 0);
            cv::threshold(enhanced, enhanced, 218, 255, cv::THRESH_BINARY);
        } break;

        case ChannelType::GREEN: {
            cv::GaussianBlur(src_equalized, enhanced, cv::Size(25,25), 0, 0);
            cv::threshold(enhanced, enhanced, 208, 255, cv::THRESH_BINARY);
        } break;

        case ChannelType::RED: {
            cv::GaussianBlur(src_equalized, enhanced, cv::Size(25,25), 0, 0);
            cv::threshold(enhanced, enhanced, 208, 255, cv::THRESH_BINARY);
        } break;

        default: {
            std::cerr << "Invalid channel type" << std::endl;
            return false;
        }
    }
    *dst = enhanced;
    return true;
}

/* Find the contours in the image */
void contourCalc(cv::Mat src, cv::Mat *dst, std::vector<std::vector<cv::Point>> *contour_data) {

    // Blur the image
    cv::Mat src_blur;
    cv::blur(src, src_blur, cv::Size(3,3) );

    cv::Mat canny_output;
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;

    // Detect edges using canny
    cv::Canny(src_blur, canny_output, 0, 255, 3);

    // Find contours
    cv::findContours(canny_output, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
    *contour_data = contours;

    // Draw contours
    cv::Mat drawing = cv::Mat::zeros(canny_output.size(), CV_8UC3);
    cv::RNG rng(12345);
    for (size_t i = 0; i< contours.size(); i++) {
        cv::Scalar color = cv::Scalar(rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255));
        cv::drawContours(drawing, contours, (int)i, color, 2, 8, hierarchy, 0, cv::Point());
    }
    *dst = drawing;
}

/* Remove redundant contours */
void removeRedundantContours(std::vector<std::vector<cv::Point>> contours, 
                                unsigned int contour_area_lower_limit, 
                                unsigned int norm_upper_limit, 
                                std::vector<std::vector<cv::Point>> *res_contours) {

    std::vector<cv::Moments> mu(contours.size());
    std::vector<std::vector<cv::Point>> contours_poly(contours.size());
    std::vector<cv::Point2f> mc(contours.size());

    for (size_t i = 0; i < contours.size(); i++) {
        approxPolyDP(cv::Mat(contours[i]), contours_poly[i], 3, true);
        mu[i] = moments(contours_poly[i], false);
        mc[i] = cv::Point2f(static_cast<float>(mu[i].m10/mu[i].m00), 
                                static_cast<float>(mu[i].m01/mu[i].m00));
    }

    std::vector<std::vector<cv::Point>> temp_contours;
    size_t k = 0;
    for (size_t i = 0; i < contours_poly.size();) {
        if (mu[i].m00 >= contour_area_lower_limit) {
            temp_contours.push_back(contours_poly[i]);
            size_t j = i+1;
            for (; j < contours_poly.size(); j++) {
                if (cv::norm(mc[i]-mc[j]) <= norm_upper_limit) {
                    if (mu[j].m00 >= contour_area_lower_limit) {
                        temp_contours[k].insert(temp_contours[k].end(), 
                                contours_poly[j].begin(), contours_poly[j].end());
                    }
                } else {
                    break;
                }
            }
            i = j;
            k++;
        } else {
            i++;
        }
    }

    std::vector<std::vector<cv::Point>> temp_contours_poly(temp_contours.size());
    std::vector<cv::Moments> temp_mu(temp_contours.size());
    std::vector<cv::Point2f> temp_mc(temp_contours.size());
    for (size_t i = 0; i < temp_contours.size(); i++) {
        temp_mu[i] = moments(temp_contours[i], false);
        temp_mc[i] = cv::Point2f(static_cast<float>(temp_mu[i].m10/temp_mu[i].m00), 
                                static_cast<float>(temp_mu[i].m01/temp_mu[i].m00));
        approxPolyDP(cv::Mat(temp_contours[i]), temp_contours_poly[i], 3, true);
    }
    *res_contours = temp_contours_poly;
}

/* Classify Nuclei and Astrocytes */
void classifyNucleiAndAstrocytes(std::vector<std::vector<cv::Point>> blue_contours, 
                                    cv::Mat blue_green_intersection,
                                    std::vector<std::vector<cv::Point>> *result_contours) {

    std::vector<std::vector<cv::Point>> temp_contours;

    // Eliminate small contours via contour area calculation
    for (size_t i = 0; i < blue_contours.size(); i++) {
        if (arcLength(blue_contours[i], true) >= 250) {
            temp_contours.push_back(blue_contours[i]);
        }
    }
    unsigned int total_cells = temp_contours.size();

    // Calculate the aspect ratio of each blue contour, 
    // categorize the astrocytes - whose aspect ratio is not close to 1.
    for (auto it = temp_contours.begin(); it != temp_contours.end();) {
        cv::RotatedRect ellipse = fitEllipse(cv::Mat(*it));
        float aspect_ratio = float(ellipse.size.width)/ellipse.size.height;
        if (aspect_ratio <= 0.5) {
            it = temp_contours.erase(it);
        } else {
            it++;
        }
    }
    unsigned int cells_left = temp_contours.size();
    
    // Find the coverage ratio for each contour on the blue-green intersection mask
    for (auto it = temp_contours.begin(); it != temp_contours.end();) {
        std::vector<std::vector<cv::Point>> specific_contour (1, *it);
        cv::Mat drawing = cv::Mat::zeros(blue_green_intersection.size(), CV_8U);
        cv::drawContours(drawing, specific_contour, -1, cv::Scalar::all(255), -1, 8, 
                                            std::vector<cv::Vec4i>(), 0, cv::Point());
        int contour_count_before = countNonZero(drawing);
        cv::Mat contour_intersection;
        bitwise_and(drawing, blue_green_intersection, contour_intersection);
        int contour_count_after = countNonZero(contour_intersection);

        float coverage_ratio = ((float)contour_count_after)/contour_count_before;
        if (coverage_ratio < 0.35) {
            it = temp_contours.erase(it);
        } else {
            it++;
        }
    }
    std:: cout << "cell count = " << total_cells 
                << " , cell count after aspect ratio elimination = " << cells_left 
                << " , nuclei count = " << temp_contours.size() << std::endl;
    *result_contours = temp_contours;
}

/* Process the images inside each directory */
bool processDir(std::string dir_name) {

    DIR *read_dir = opendir(dir_name.c_str());
    if (!read_dir) {
        std::cerr << "Could not open directory '" << dir_name << "'" << std::endl;
        return false;
    }

    // Count the number of images
    uint8_t z_count = 0;
    struct dirent *dir = NULL;
    while ((dir = readdir(read_dir))) {
        if (!strcmp (dir->d_name, ".") || !strcmp (dir->d_name, "..")) {
            continue;
        }
        z_count++;
    }
    closedir(read_dir);

    if (z_count < NUM_Z_LAYERS) {
        std::cerr << "Not enough z layers in the image." << std::endl;
        return false;
    }

    // Extract the input directory name
    std::istringstream iss(dir_name);
    std::string token;
    getline(iss, token, '/');
    getline(iss, token, '/');

    // Create the output directory
    std::string out_directory = "result/" + token + "/";
    struct stat st = {0};
    if (stat(out_directory.c_str(), &st) == -1) {
        mkdir(out_directory.c_str(), 0700);
    }

    std::vector<cv::Mat> blue(NUM_Z_LAYERS), green(NUM_Z_LAYERS), red(NUM_Z_LAYERS);
    for (uint8_t z_index = 1; z_index <= z_count; z_index++) {

        // Create the input filename and rgb stream output filenames
        std::string in_filename;
        if (z_count < 10) {
            in_filename  = dir_name + token + "_z" + std::to_string(z_index) + "c1+2+3.tif";
        } else {
            if (z_index < 10) {
                in_filename  = dir_name + token + "_z0" + std::to_string(z_index) + "c1+2+3.tif";
            } else if (z_index < 100) {
                in_filename  = dir_name + token + "_z" + std::to_string(z_index) + "c1+2+3.tif";
            } else { // assuming number of z plane layers will never exceed 99
                std::cerr << "Does not support more than 99 z layers curently" << std::endl;
                return false;
            }
        }

        // Extract the bgr streams for each input image
        cv::Mat img = cv::imread(in_filename.c_str());
        if (img.empty()) {
            std::cerr << "Invalid input filename" << std::endl;
            return false;
        }

        std::vector<cv::Mat> channel(3);
        cv::split(img, channel);

        blue[(z_index-1)%NUM_Z_LAYERS] = channel[0];
        green[(z_index-1)%NUM_Z_LAYERS] = channel[1];
        red[(z_index-1)%NUM_Z_LAYERS] = channel[2];

        // Merge, enhance, segment and extract features for a certain number of Z layers
        if (z_index >= NUM_Z_LAYERS) {

            // Blue channel
            cv::Mat blue_merge, blue_enhanced, blue_contour;
            std::vector<std::vector<cv::Point>> contours_blue, contours_blue_ref;
            cv::merge(blue, blue_merge);
            std::string out_blue = out_directory + "z" + std::to_string(z_index-NUM_Z_LAYERS+1) + 
                                            "_blue_" + std::to_string(NUM_Z_LAYERS) + "layers.tif";
            cv::imwrite(out_blue.c_str(), blue_merge);
            if(!enhanceImage(blue_merge, ChannelType::BLUE, &blue_enhanced)) {
                return false;
            }
            out_blue.insert(out_blue.find_first_of("."), "_enhanced", 9);
            cv::imwrite(out_blue.c_str(), blue_enhanced);
            contourCalc(blue_enhanced, &blue_contour, &contours_blue);
            out_blue.insert(out_blue.find_first_of("."), "_contours", 9);
            cv::imwrite(out_blue.c_str(), blue_contour);
            removeRedundantContours(contours_blue, 12, 80, &contours_blue_ref);

            // Green channel
            cv::Mat green_merge, green_enhanced, green_contour;
            std::vector<std::vector<cv::Point>> contours_green;
            cv::merge(green, green_merge);
            std::string out_green = out_directory + "z" + std::to_string(z_index-NUM_Z_LAYERS+1) + 
                                            "_green_" + std::to_string(NUM_Z_LAYERS) + "layers.tif";
            cv::imwrite(out_green.c_str(), green_merge);
            if(!enhanceImage(green_merge, ChannelType::GREEN, &green_enhanced)) {
                return false;
            }
            out_green.insert(out_green.find_first_of("."), "_enhanced", 9);
            cv::imwrite(out_green.c_str(), green_enhanced);
            contourCalc(green_enhanced, &green_contour, &contours_green);
            out_green.insert(out_green.find_first_of("."), "_contours", 9);
            cv::imwrite(out_green.c_str(), green_contour);

            // Red channel
            cv::Mat red_merge, red_enhanced, red_contour;
            std::vector<std::vector<cv::Point>> contours_red, contours_red_ref;
            cv::merge(red, red_merge);
            std::string out_red = out_directory + "z" + std::to_string(z_index-NUM_Z_LAYERS+1) + 
                                                "_red_" + std::to_string(NUM_Z_LAYERS) + "layers.tif";
            cv::imwrite(out_red.c_str(), red_merge);
            if(!enhanceImage(red_merge, ChannelType::RED, &red_enhanced)) {
                return false;
            }
            out_red.insert(out_red.find_first_of("."), "_enhanced", 9);
            cv::imwrite(out_red.c_str(), red_enhanced);
            contourCalc(red_enhanced, &red_contour, &contours_red);
            out_red.insert(out_red.find_first_of("."), "_contours", 9);
            cv::imwrite(out_red.c_str(), red_contour);
            removeRedundantContours(contours_red, 0, 10, &contours_red_ref);

            // Classify nuclei and astrocytes
            cv::Mat blue_green_intersection;
            bitwise_and(blue_enhanced, green_enhanced, blue_green_intersection);

            std::vector<std::vector<cv::Point>> temp_blue_contours;
            classifyNucleiAndAstrocytes(contours_blue_ref, blue_green_intersection, &temp_blue_contours);

            // Draw contours
            cv::Mat drawing = cv::Mat::zeros(blue_contour.size(), CV_32S);
            cv::RNG rng(12345);
            for (size_t i = 0; i< temp_blue_contours.size(); i++) {
                cv::Scalar color = cv::Scalar(rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255));
                cv::drawContours(drawing, temp_blue_contours, (int)i, color, 2, 8, 
                                            std::vector<cv::Vec4i>(), 0, cv::Point());
            }
            out_blue.insert(out_blue.find_first_of("."), "_classification", 15);
            cv::imwrite(out_blue.c_str(), drawing);

            // Count and calculate area of synapses

        }
    }
    return true;
}

int main(int argc, char *argv[]) {
    std::string str(argv[1]);
    if (!processDir(str)) {
        return -1;
    }
    return 0;
}

