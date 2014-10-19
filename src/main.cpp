#include <iostream>
#include <dirent.h>
#include <sys/stat.h>
#include <fstream>

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"

#define NUM_Z_LAYERS            3   // Merge a certain number of z layers
#define NUM_SYNAPSE_AREA_BINS   21  // Number of bins
#define SYNAPSE_BIN_AREA        25  // Bin area
#define DEBUG_FLAG              1   // Debug flag for image channels

/* Channel type */
enum class ChannelType : unsigned char {
    BLUE = 0,
    GREEN,
    RED_LOW,
    RED_HIGH
};

/* Hierarchy type */
enum class HierarchyType : unsigned char {
    INVALID_CNTR = 0,
    CHILD_CNTR,
    PARENT_CNTR
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
            cv::threshold(enhanced, enhanced, 200, 255, cv::THRESH_BINARY);
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
void contourCalc(cv::Mat src, ChannelType channel_type, double min_area, cv::Mat *dst, 
                    std::vector<std::vector<cv::Point>> *contours, std::vector<cv::Vec4i> *hierarchy, 
                    std::vector<HierarchyType> *validity_mask, std::vector<double> *parent_area) {

    cv::Mat temp_src;
    src.copyTo(temp_src);
    switch(channel_type) {
        case ChannelType::BLUE: {
            findContours(temp_src, *contours, *hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        } break;

        case ChannelType::RED_LOW : 
        case ChannelType::RED_HIGH: {
            findContours(temp_src, *contours, *hierarchy, cv::RETR_CCOMP, cv::CHAIN_APPROX_SIMPLE);
        } break;

        case ChannelType::GREEN: default: return;
    }

    *dst = cv::Mat::zeros(temp_src.size(), CV_8UC3);
    if (!contours->size()) return;
    validity_mask->assign(contours->size(), HierarchyType::INVALID_CNTR);
    parent_area->assign(contours->size(), 0.0);

    // Keep the contours whose size is >= than min_area
    cv::RNG rng(12345);
    for (int index = 0 ; index < (int)contours->size(); index++) {
        if ((*hierarchy)[index][3] > -1) continue; // ignore child
        auto cntr_external = (*contours)[index];
        double area_external = fabs(contourArea(cv::Mat(cntr_external)));
        if (area_external < min_area) continue;

        std::vector<int> cntr_list;
        cntr_list.push_back(index);

        int index_hole = (*hierarchy)[index][2];
        double area_hole = 0.0;
        while (index_hole > -1) {
            std::vector<cv::Point> cntr_hole = (*contours)[index_hole];
            double temp_area_hole = fabs(contourArea(cv::Mat(cntr_hole)));
            if (temp_area_hole) {
                cntr_list.push_back(index_hole);
                area_hole += temp_area_hole;
            }
            index_hole = (*hierarchy)[index_hole][0];
        }
        double area_contour = area_external - area_hole;
        if (area_contour >= min_area) {
            (*validity_mask)[cntr_list[0]] = HierarchyType::PARENT_CNTR;
            (*parent_area)[cntr_list[0]] = area_contour;
            for (unsigned int i = 1; i < cntr_list.size(); i++) {
                (*validity_mask)[cntr_list[i]] = HierarchyType::CHILD_CNTR;
            }
            cv::Scalar color = cv::Scalar(rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255));
            drawContours(*dst, *contours, index, color, cv::FILLED, cv::LINE_8, *hierarchy);
        }
    }
}

/* Classify Neurons and Astrocytes */
void classifyNeuronsAndAstrocytes(std::vector<std::vector<cv::Point>> blue_contours,
                                    std::vector<HierarchyType> blue_contour_mask,
                                    cv::Mat blue_green_intersection,
                                    std::vector<std::vector<cv::Point>> *astrocyte_contours,
                                    std::vector<std::vector<cv::Point>> *neuron_contours) {

    for (size_t i = 0; i < blue_contours.size(); i++) {

        if (blue_contour_mask[i] != HierarchyType::PARENT_CNTR) continue;

        // Eliminate small contours via contour arc calculation
        if ((arcLength(blue_contours[i], true) >= 250) && (blue_contours[i].size() >= 5)) {

            // Determine whether cell is a neuron by calculating blue-green coverage area
            std::vector<std::vector<cv::Point>> specific_contour (1, blue_contours[i]);
            cv::Mat drawing = cv::Mat::zeros(blue_green_intersection.size(), CV_8U);
            cv::drawContours(drawing, specific_contour, -1, cv::Scalar::all(255), -1, 8, 
                                                std::vector<cv::Vec4i>(), 0, cv::Point());
            int contour_count_before = countNonZero(drawing);
            cv::Mat contour_intersection;
            bitwise_and(drawing, blue_green_intersection, contour_intersection);
            int contour_count_after = countNonZero(contour_intersection);
            float coverage_ratio = ((float)contour_count_after)/contour_count_before;
            if (coverage_ratio < 0.25) {
                astrocyte_contours->push_back(blue_contours[i]);
            } else {
                // Calculate the aspect ratio of the blue contour,
                // categorize as astrocytes if aspect ratio is very low.
                cv::RotatedRect min_area_rect = minAreaRect(cv::Mat(blue_contours[i]));
                float aspect_ratio = float(min_area_rect.size.width)/min_area_rect.size.height;
                if (aspect_ratio > 1.0) {
                    aspect_ratio = 1.0/aspect_ratio;
                }
                if (aspect_ratio <= 0.1) {
                    astrocyte_contours->push_back(blue_contours[i]);
                } else {
                    neuron_contours->push_back(blue_contours[i]);
                }
            }
        }
    }
}

/* Astrocytes-neurons separation metrics */
void neuronAstroSepMetrics(std::vector<std::vector<cv::Point>> astrocyte_contours, 
                                std::vector<std::vector<cv::Point>> neuron_contours,
                                float *mean_astrocyte_proximity_cnt,
                                float *stddev_astrocyte_proximity_cnt) {

    // Calculate the mid point of all astrocytes
    std::vector<cv::Point2f> mc_astrocyte(astrocyte_contours.size());
    for (size_t i = 0; i < astrocyte_contours.size(); i++) {
        cv::Moments mu = moments(astrocyte_contours[i], true);
        mc_astrocyte[i] = cv::Point2f(static_cast<float>(mu.m10/mu.m00), 
                                            static_cast<float>(mu.m01/mu.m00));
    }

    // Calculate the mid point of all neurons
    std::vector<cv::Point2f> mc_neuron(neuron_contours.size());
    for (size_t i = 0; i < neuron_contours.size(); i++) {
        cv::Moments mu = moments(neuron_contours[i], true);
        mc_neuron[i] = cv::Point2f(static_cast<float>(mu.m10/mu.m00), 
                                            static_cast<float>(mu.m01/mu.m00));
    }

    // Compute the normal distribution parameters of astrocyte count per neuron
    std::vector<float> count(neuron_contours.size(), 0.0);
    for (size_t i = 0; i < neuron_contours.size(); i++) {
        for (size_t j = 0; j < astrocyte_contours.size(); j++) {
            if (cv::norm(mc_neuron[i] - mc_astrocyte[j]) <= 500.0) {
                count[i]++;
            }
        }
    }
    cv::Scalar mean, stddev;
    cv::meanStdDev(count, mean, stddev);
    *mean_astrocyte_proximity_cnt = static_cast<float>(mean.val[0]);
    *stddev_astrocyte_proximity_cnt = static_cast<float>(stddev.val[0]);
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

        // Manipulate RGB channels and extract features for a certain number of Z layers
        if (z_index >= NUM_Z_LAYERS) {

            /* Gather RGB channel information needed for feature extraction */

            // Blue channel
            cv::Mat blue_merge, blue_enhanced, blue_segmented;
            std::vector<std::vector<cv::Point>> contours_blue;
            std::vector<cv::Vec4i> hierarchy_blue;
            std::vector<HierarchyType> blue_contour_mask;
            std::vector<double> blue_contour_area;

            cv::merge(blue, blue_merge);
            std::string out_blue = out_directory + "z" + std::to_string(z_index-NUM_Z_LAYERS+1) + 
                                            "_blue_" + std::to_string(NUM_Z_LAYERS) + "layers.tif";
            if (DEBUG_FLAG) cv::imwrite(out_blue.c_str(), blue_merge);
            if(!enhanceImage(blue_merge, ChannelType::BLUE, &blue_enhanced)) {
                return false;
            }
            out_blue.insert(out_blue.find_first_of("."), "_enhanced", 9);
            cv::imwrite(out_blue.c_str(), blue_enhanced);
            contourCalc(blue_enhanced, ChannelType::BLUE, 50.00, &blue_segmented, &contours_blue, 
                                &hierarchy_blue, &blue_contour_mask, &blue_contour_area);
            out_blue.insert(out_blue.find_first_of("."), "_segmented", 10);
            cv::imwrite(out_blue.c_str(), blue_segmented);

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

            // Red channel - Lower intensity
            cv::Mat red_merge, red_low_enhanced, red_low_segmented;
            std::vector<std::vector<cv::Point>> contours_red_low;
            std::vector<cv::Vec4i> hierarchy_red_low;
            std::vector<HierarchyType> red_low_contour_mask;
            std::vector<double> red_low_contour_area;

            cv::merge(red, red_merge);
            std::string out_red_low = out_directory + "z" + std::to_string(z_index-NUM_Z_LAYERS+1) + 
                                                "_red_low_" + std::to_string(NUM_Z_LAYERS) + "layers.tif";
            if (DEBUG_FLAG) cv::imwrite(out_red_low.c_str(), red_merge);
            if(!enhanceImage(red_merge, ChannelType::RED_LOW, &red_low_enhanced)) {
                return false;
            }
            out_red_low.insert(out_red_low.find_first_of("."), "_enhanced", 9);
            cv::imwrite(out_red_low.c_str(), red_low_enhanced);
            contourCalc(red_low_enhanced, ChannelType::RED_LOW, 1.0, &red_low_segmented, &contours_red_low, 
                                &hierarchy_red_low, &red_low_contour_mask, &red_low_contour_area);
            out_red_low.insert(out_red_low.find_first_of("."), "_segmented", 10);
            cv::imwrite(out_red_low.c_str(), red_low_segmented);

            // Red channel - High intensity
            cv::Mat red_high_enhanced, red_high_segmented;
            std::vector<std::vector<cv::Point>> contours_red_high;
            std::vector<cv::Vec4i> hierarchy_red_high;
            std::vector<HierarchyType> red_high_contour_mask;
            std::vector<double> red_high_contour_area;

            std::string out_red_high = out_directory + "z" + std::to_string(z_index-NUM_Z_LAYERS+1) + 
                                                "_red_high_" + std::to_string(NUM_Z_LAYERS) + "layers.tif";
            if(!enhanceImage(red_merge, ChannelType::RED_HIGH, &red_high_enhanced)) {
                return false;
            }
            out_red_high.insert(out_red_high.find_first_of("."), "_enhanced", 9);
            cv::imwrite(out_red_high.c_str(), red_high_enhanced);
            contourCalc(red_high_enhanced, ChannelType::RED_HIGH, 1.0, &red_high_segmented, &contours_red_high, 
                                &hierarchy_red_high, &red_high_contour_mask, &red_high_contour_area);
            out_red_high.insert(out_red_high.find_first_of("."), "_segmented", 10);
            cv::imwrite(out_red_high.c_str(), red_high_segmented);


            /** Extract multi-dimensional features for analysis **/

            // Blue-green channel intersection
            cv::Mat blue_green_intersection;
            bitwise_and(blue_enhanced, green_enhanced, blue_green_intersection);
            out_green.insert(out_green.find_first_of("."), "_blue_intersection", 18);
            if (DEBUG_FLAG) cv::imwrite(out_green.c_str(), blue_green_intersection);

            // Classify astrocytes and neurons
            std::vector<std::vector<cv::Point>> astrocyte_contours, neuron_contours;
            classifyNeuronsAndAstrocytes(contours_blue, blue_contour_mask, blue_green_intersection, 
                                                &astrocyte_contours, &neuron_contours);
            cv::RNG rng(12345);
            cv::Mat drawing_blue = cv::Mat::zeros(blue_enhanced.size(), CV_32S);
            for (size_t i = 0; i< neuron_contours.size(); i++) {
                cv::Scalar color = cv::Scalar(rng.uniform(0, 255), 
                                            rng.uniform(0,255), rng.uniform(0,255));
                cv::drawContours(drawing_blue, neuron_contours, (int)i, color, 2, 8, 
                                            std::vector<cv::Vec4i>(), 0, cv::Point());
            }
            out_blue.insert(out_blue.find_first_of("."), "_neurons", 8);
            cv::imwrite(out_blue.c_str(), drawing_blue);
            data_stream << dir_name << "," << std::to_string(z_index-NUM_Z_LAYERS+1) << "," 
                        << astrocyte_contours.size() << "," << neuron_contours.size() << ",";

            // Calculate metrics for astrocytes-neurons separation
            float mean_astrocyte_proximity_cnt = 0.0, stddev_astrocyte_proximity_cnt = 0.0;
            neuronAstroSepMetrics(astrocyte_contours, neuron_contours, 
                                    &mean_astrocyte_proximity_cnt, 
                                    &stddev_astrocyte_proximity_cnt);
            data_stream << mean_astrocyte_proximity_cnt << "," 
                        << stddev_astrocyte_proximity_cnt << ",";


            // Classify synapses
            std::vector<std::vector<cv::Point>> temp_red_low_contours, temp_red_high_contours;
            std::string synapse_low_bin_cnt, synapse_high_bin_cnt;
            classifySynapses(contours_red_low, &temp_red_low_contours, &synapse_low_bin_cnt);
            classifySynapses(contours_red_high, &temp_red_high_contours, &synapse_high_bin_cnt);
            cv::Mat drawing_red_low = cv::Mat::zeros(red_low_segmented.size(), CV_32S);
            for (size_t i = 0; i< temp_red_low_contours.size(); i++) {
                cv::Scalar color = cv::Scalar(rng.uniform(0, 255), 
                                            rng.uniform(0,255), rng.uniform(0,255));
                cv::drawContours(drawing_red_low, temp_red_low_contours, (int)i, color, 2, 8, 
                                            std::vector<cv::Vec4i>(), 0, cv::Point());
            }
            out_red_low.insert(out_red_low.find_first_of("."), "_synapses", 9);
            cv::imwrite(out_red_low.c_str(), drawing_red_low);
            data_stream << temp_red_low_contours.size() << "," << temp_red_high_contours.size() 
                        << "," << synapse_low_bin_cnt << std::endl;
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
    data_stream << "path/image,frame,astrocyte count,neuron count,\
                    astrocytes per neuron - mean,astrocytes per neuron - std dev,\
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

