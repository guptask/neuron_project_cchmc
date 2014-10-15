#include <iostream>
#include <dirent.h>
#include <sys/stat.h>
#include <fstream>

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"

#define NUM_Z_LAYERS           3   // Merge a certain number of z layers
#define NUM_SYNAPSE_AREA_BINS  20  // Number of bins
#define SYNAPSE_BIN_AREA       25  // Bin area
#define DEBUG_FLAG             1   // Debug flag for image channels

/* Channel type */
enum class ChannelType : unsigned char {
    BLUE = 0,
    GREEN,
    RED_LOW,
    RED_HIGH
};

/* Enhance the image */
bool enhanceImage(cv::Mat src, ChannelType channel_type, cv::Mat *dst) {

    // Convert to grayscale
    cv::Mat src_gray;
    cvtColor (src, src_gray, cv::COLOR_BGR2GRAY);

    // Enhance the image using Gaussian blur and thresholding
    cv::Mat enhanced;
    switch(channel_type) {
        case ChannelType::BLUE: {
            equalizeHist(src_gray, src_gray);
            cv::GaussianBlur(src_gray, enhanced, cv::Size(5,5), 0, 0);
            cv::threshold(enhanced, enhanced, 218, 255, cv::THRESH_BINARY);
        } break;

        case ChannelType::GREEN: {
            cv::GaussianBlur(src_gray, src_gray, cv::Size(3,3), 0, 0);
            cv::threshold(src_gray, enhanced, 25, 255, cv::THRESH_BINARY);
        } break;

        case ChannelType::RED_LOW: {
            // Enhance the red channel low intensities
            cv::Mat red_low = src_gray;

            // Create the mask
            cv::threshold(src_gray, src_gray, 20, 255, cv::THRESH_TOZERO);
            cv::threshold(src_gray, src_gray, 150, 255, cv::THRESH_TRUNC);
            bitwise_not(src_gray, src_gray);
            cv::GaussianBlur(src_gray, enhanced, cv::Size(25,25), 0, 0);
            cv::threshold(enhanced, enhanced, 210, 255, cv::THRESH_BINARY);

            // Enhance the low intensity features
            cv::Mat red_low_gauss;
            cv::GaussianBlur(red_low, red_low_gauss, cv::Size(25,25), 0, 0);
            bitwise_and(red_low_gauss, enhanced, enhanced);
            cv::threshold(enhanced, enhanced, 240, 255, cv::THRESH_TOZERO_INV);
            cv::threshold(enhanced, enhanced, 50, 255, cv::THRESH_BINARY);
        } break;

        case ChannelType::RED_HIGH: {
            // Enhance the red channel higher intensities

            // Create the mask
            cv::threshold(src_gray, src_gray, 20, 255, cv::THRESH_TOZERO);
            cv::threshold(src_gray, src_gray, 150, 255, cv::THRESH_TRUNC);
            bitwise_not(src_gray, src_gray);
            cv::GaussianBlur(src_gray, enhanced, cv::Size(25,25), 0, 0);
            cv::threshold(enhanced, enhanced, 210, 255, cv::THRESH_BINARY);

            // Invert the mask
            bitwise_not(enhanced, enhanced);
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

/* Classify Neurons and Astrocytes */
void classifyNeuronsAndAstrocytes(std::vector<std::vector<cv::Point>> blue_contours,
                                    cv::Mat blue_green_intersection,
                                    std::vector<std::vector<cv::Point>> *result_contours,
                                    unsigned int *total_cell_cnt,
                                    unsigned int *neuron_cnt) {

    std::vector<std::vector<cv::Point>> temp_contours;

    // Eliminate small contours via contour area calculation
    for (size_t i = 0; i < blue_contours.size(); i++) {
        if ((arcLength(blue_contours[i], true) >= 250) && (blue_contours[i].size() > 10)) {
            temp_contours.push_back(blue_contours[i]);
        }
    }
    *total_cell_cnt = temp_contours.size();

    // Calculate the aspect ratio of each blue contour, 
    // categorize the astrocytes - whose aspect ratio is not close to 1.
    /*for (auto it = temp_contours.begin(); it != temp_contours.end();) {
        cv::RotatedRect ellipse = fitEllipse(cv::Mat(*it));
        float aspect_ratio = float(ellipse.size.width)/ellipse.size.height;
        if (aspect_ratio <= 0.5) {
            it = temp_contours.erase(it);
        } else {
            it++;
        }
    }*/

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
    *neuron_cnt = temp_contours.size();
    *result_contours = temp_contours;
}

/* Classify synapses */
void classifySynapses(std::vector<std::vector<cv::Point>> red_contours, 
                        std::vector<std::vector<cv::Point>> *result_contours,
                        std::string *out_data) {

    std::vector<cv::Moments> mu(red_contours.size());
    std::vector<cv::Point2f> mc(red_contours.size());

    std::vector<std::vector<cv::Point>> temp_contours;
    std::vector<unsigned int> count(NUM_SYNAPSE_AREA_BINS);
    size_t i = 0;
    while (i < red_contours.size()) {
        mu[i] = moments(red_contours[i], false);
        mc[i] = cv::Point2f(static_cast<float>(mu[i].m10/mu[i].m00), 
                                static_cast<float>(mu[i].m01/mu[i].m00));
        if (mu[i].m00) {
            unsigned int round_area = (unsigned int)mu[i].m00;
            unsigned int index = 
                (round_area/SYNAPSE_BIN_AREA < NUM_SYNAPSE_AREA_BINS) ? 
                                round_area/SYNAPSE_BIN_AREA : NUM_SYNAPSE_AREA_BINS-1;
            count[index]++;
            temp_contours.push_back(red_contours[i]);
        }
        i++;
    }

    for (unsigned int i = 0; i < count.size(); i++) {
        *out_data += std::to_string(count[i]) + ",";
    }

    *result_contours = temp_contours;
}

/* Process the images inside each directory */
bool processDir(std::string dir_name, std::string out_file) {

    /* Create the data output file for images that were processed */
    std::ofstream data_stream;
    data_stream.open(out_file, std::ios::app);
    if (!data_stream.is_open()) {
        std::cerr << "Could not open the data output file." << std::endl;
        return false;
    }

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
            if (DEBUG_FLAG) cv::imwrite(out_blue.c_str(), blue_merge);
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
            cv::Mat green_merge, green_enhanced;
            cv::merge(green, green_merge);
            std::string out_green = out_directory + "z" + std::to_string(z_index-NUM_Z_LAYERS+1) + 
                                            "_green_" + std::to_string(NUM_Z_LAYERS) + "layers.tif";
            if (DEBUG_FLAG) cv::imwrite(out_green.c_str(), green_merge);
            if(!enhanceImage(green_merge, ChannelType::GREEN, &green_enhanced)) {
                return false;
            }
            out_green.insert(out_green.find_first_of("."), "_enhanced", 9);
            cv::imwrite(out_green.c_str(), green_enhanced);

            // Red channel
            // Lower intensity
            cv::Mat red_merge, red_low_enhanced, red_low_contour;
            std::vector<std::vector<cv::Point>> contours_red_low, contours_red_low_ref;
            cv::merge(red, red_merge);
            std::string out_red_low = out_directory + "z" + std::to_string(z_index-NUM_Z_LAYERS+1) + 
                                                "_red_low_" + std::to_string(NUM_Z_LAYERS) + "layers.tif";
            if (DEBUG_FLAG) cv::imwrite(out_red_low.c_str(), red_merge);
            if(!enhanceImage(red_merge, ChannelType::RED_LOW, &red_low_enhanced)) {
                return false;
            }
            out_red_low.insert(out_red_low.find_first_of("."), "_enhanced", 9);
            cv::imwrite(out_red_low.c_str(), red_low_enhanced);
            contourCalc(red_low_enhanced, &red_low_contour, &contours_red_low);
            out_red_low.insert(out_red_low.find_first_of("."), "_contours", 9);
            cv::imwrite(out_red_low.c_str(), red_low_contour);
            removeRedundantContours(contours_red_low, 1, 10, &contours_red_low_ref);

            // High intensity
            cv::Mat red_high_enhanced, red_high_contour;
            std::vector<std::vector<cv::Point>> contours_red_high, contours_red_high_ref;
            std::string out_red_high = out_directory + "z" + std::to_string(z_index-NUM_Z_LAYERS+1) + 
                                                "_red_high_" + std::to_string(NUM_Z_LAYERS) + "layers.tif";
            if(!enhanceImage(red_merge, ChannelType::RED_HIGH, &red_high_enhanced)) {
                return false;
            }
            out_red_high.insert(out_red_high.find_first_of("."), "_enhanced", 9);
            cv::imwrite(out_red_high.c_str(), red_high_enhanced);
            contourCalc(red_high_enhanced, &red_high_contour, &contours_red_high);
            out_red_high.insert(out_red_high.find_first_of("."), "_contours", 9);
            cv::imwrite(out_red_high.c_str(), red_high_contour);
            removeRedundantContours(contours_red_high, 1, 10, &contours_red_high_ref);


            /** Classify neurons and astrocytes **/
            cv::RNG rng(12345);
            cv::Mat blue_green_intersection;
            unsigned int total_cell_cnt = 0, neuron_cnt = 0;
            std::vector<std::vector<cv::Point>> temp_blue_contours;
            bitwise_and(blue_enhanced, green_enhanced, blue_green_intersection);
            out_green.insert(out_green.find_first_of("."), "_blue_intersection", 18);
            if (DEBUG_FLAG) cv::imwrite(out_green.c_str(), blue_green_intersection);
            classifyNeuronsAndAstrocytes(contours_blue_ref, blue_green_intersection, 
                                            &temp_blue_contours, &total_cell_cnt, &neuron_cnt);
            data_stream << dir_name << "," << std::to_string(z_index-NUM_Z_LAYERS+1) << "," 
                                << total_cell_cnt << "," << neuron_cnt << ",";

            // Draw contours
            cv::Mat drawing_blue = cv::Mat::zeros(blue_contour.size(), CV_32S);
            for (size_t i = 0; i< temp_blue_contours.size(); i++) {
                cv::Scalar color = cv::Scalar(rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255));
                cv::drawContours(drawing_blue, temp_blue_contours, (int)i, color, 2, 8, 
                                            std::vector<cv::Vec4i>(), 0, cv::Point());
            }
            out_blue.insert(out_blue.find_first_of("."), "_classification", 15);
            cv::imwrite(out_blue.c_str(), drawing_blue);

            /** Classify synapses **/
            std::vector<std::vector<cv::Point>> temp_red_low_contours, temp_red_high_contours;
            std::string synapse_low_bin_cnt, synapse_high_bin_cnt;
            classifySynapses(contours_red_low_ref, &temp_red_low_contours, &synapse_low_bin_cnt);
            classifySynapses(contours_red_high_ref, &temp_red_high_contours, &synapse_high_bin_cnt);
            data_stream << temp_red_low_contours.size() << "," << temp_red_high_contours.size() 
                                << "," << synapse_low_bin_cnt << std::endl;

            // Draw contours
            cv::Mat drawing_red_low = cv::Mat::zeros(red_low_contour.size(), CV_32S);
            for (size_t i = 0; i< temp_red_low_contours.size(); i++) {
                cv::Scalar color = cv::Scalar(rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255));
                cv::drawContours(drawing_red_low, temp_red_low_contours, (int)i, color, 2, 8, 
                                            std::vector<cv::Vec4i>(), 0, cv::Point());
            }
            out_red_low.insert(out_red_low.find_first_of("."), "_classification", 15);
            cv::imwrite(out_red_low.c_str(), drawing_red_low);
        }
    }
    data_stream.close();
    return true;
}

/* Main - create the threads and start the processing */
int main(int argc, char *argv[]) {

    /* Check for argument count */
    if (argc != 5) {
        std::cerr << "Invalid number of arguments." << std::endl;
        return -1;
    }

    /* Read the path to the data */
    std::string path(argv[1]);

    /* Read the list of directories to process */
    std::vector<std::string> files;
    FILE *file = fopen(argv[2], "r");
    if (!file) {
        std::cerr << "Could not open the file list." << std::endl;
        return -1;
    }
    char line[128];
    while (fgets(line, sizeof(line), file) != NULL) {
        line[strlen(line)-1] = '/';
        std::string temp_str(line);
        std::string image_name = path + temp_str;
        files.push_back(image_name);
    }
    fclose(file);

    /* Create the error log for images that could not be processed */
    std::ofstream err_file(argv[3]);
    if (!err_file.is_open()) {
        std::cerr << "Could not open the error log file." << std::endl;
        return -1;
    }

    /* Process each image directory */
    std::string out_file(argv[4]);
    std::ofstream data_stream;
    data_stream.open(out_file, std::ios::out);
    if (!data_stream.is_open()) {
        std::cerr << "Could not create the data output file." << std::endl;
        return -1;
    }
    data_stream << "path/image,frame,total cell count,neuron count,\
                total synapse count,high intensity synapse count,";
    for (unsigned int i = 0; i < NUM_SYNAPSE_AREA_BINS-1; i++) {
        data_stream << i*SYNAPSE_BIN_AREA << " <= synapse area < " 
                    << (i+1)*SYNAPSE_BIN_AREA << ",";
    }
    data_stream << "synapse area >= " 
                << (NUM_SYNAPSE_AREA_BINS-1)*SYNAPSE_BIN_AREA << std::endl;
    data_stream.close();

    for (auto& file_name : files) {
        std::cout << file_name << std::endl;
        if (!processDir(file_name, out_file)) {
            err_file << file_name << std::endl;
        }
    }
    err_file.close();

    return 0;
}

