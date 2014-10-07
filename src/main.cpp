#include <iostream>
#include <unistd.h>
#include <memory>
#include <dirent.h>
#include <sys/stat.h>

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"

#include "Moments.hpp"

int main (int argc, char *argv[]) {

    DIR *read_dir = opendir(argv[1]);
    if (!read_dir) {
        std::cerr << "Could not open directory '" << argv[1] << "'" << std::endl;
        return 0;
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

    // Extract the input directory name
    std::string str(argv[1]);
    std::istringstream iss(str);
    std::string token;
    getline(iss, token, '/');
    getline(iss, token, '/');

    // Create the output directory
    std::string out_directory = "result/" + token + "/";
    struct stat st = {0};
    if (stat(out_directory.c_str(), &st) == -1) {
        mkdir(out_directory.c_str(), 0700);
    }

    // Create the moments
    std::unique_ptr<Moments> moments = std::unique_ptr<Moments>(new Moments());

    for (uint8_t z_index = 1; z_index <= z_count; z_index++) {

        // Create the input filename and rgb stream output filenames
        std::string in_filename, out_red_filename, out_green_filename, out_blue_filename;

        if (z_index < 10) {
            in_filename  = str + token + "_z0" + std::to_string(z_index) + "c1+2+3.tif";
            out_red_filename   = out_directory + "z0" + std::to_string(z_index) + "_red.tif";
            out_green_filename = out_directory + "z0" + std::to_string(z_index) + "_green.tif";
            out_blue_filename  = out_directory + "z0" + std::to_string(z_index) + "_blue.tif";

        } else if (z_index < 100) {
            in_filename  = str + token + "_z" + std::to_string(z_index) + "c1+2+3.tif";
            out_red_filename   = out_directory + "z" + std::to_string(z_index) + "_red.tif";
            out_green_filename = out_directory + "z" + std::to_string(z_index) + "_green.tif";
            out_blue_filename  = out_directory + "z" + std::to_string(z_index) + "_blue.tif";

        } else { // assuming number of z plane layers will never exceed 99
            std::cerr << "There should not be that many z layers" << std::endl;
            return 0;
        }

        // Extract the bgr streams for each input image
        cv::Mat img = cv::imread(in_filename.c_str());
        if (img.empty()) {
            std::cerr << "Invalid input filename" << std::endl;
            return 0;
        }

        std::vector<cv::Mat> channel(3);
        cv::split(img, channel);

        cv::imwrite(out_blue_filename.c_str(), channel[0]);
        cv::imwrite(out_green_filename.c_str(), channel[1]);
        cv::imwrite(out_red_filename.c_str(), channel[2]);

        // Equalize the histogram
        equalizeHist(channel[0], channel[0]);
        equalizeHist(channel[1], channel[1]);
        equalizeHist(channel[2], channel[2]);

        // Enhance the image using Gaussian blur and thresholding
        std::vector<cv::Mat> enhanced(3);
        cv::GaussianBlur(channel[0], enhanced[0], cv::Size(5,5), 0, 0);
        cv::threshold(enhanced[0], enhanced[0], 0, 255, cv::THRESH_BINARY + cv::THRESH_OTSU);
        cv::GaussianBlur(channel[1], enhanced[1], cv::Size(11,11), 0, 0);
        cv::threshold(enhanced[1], enhanced[1], 25, 255, cv::THRESH_BINARY);
        cv::GaussianBlur(channel[2], enhanced[2], cv::Size(5,5), 0, 0);
        cv::threshold(enhanced[2], enhanced[2], 0, 255, cv::THRESH_BINARY + cv::THRESH_OTSU);

        out_blue_filename.insert(out_blue_filename.find_first_of("."), "_enhanced", 9);
        cv::imwrite(out_blue_filename.c_str(), enhanced[0]);
        out_green_filename.insert(out_green_filename.find_first_of("."), "_enhanced", 9);
        cv::imwrite(out_green_filename.c_str(), enhanced[1]);
        out_red_filename.insert(out_red_filename.find_first_of("."), "_enhanced", 9);
        cv::imwrite(out_red_filename.c_str(), enhanced[2]);

        // Moments calculation
        std::vector<cv::Mat> result(3);
        moments->apply(enhanced[0], &result[0], 0);
        moments->apply(enhanced[1], &result[1], 0);
        moments->apply(enhanced[2], &result[2], 0);

        out_blue_filename.insert(out_blue_filename.find_first_of("."), "_moment", 7);
        cv::imwrite(out_blue_filename.c_str(), result[0]);
        out_green_filename.insert(out_green_filename.find_first_of("."), "_moment", 7);
        cv::imwrite(out_green_filename.c_str(), result[1]);
        out_red_filename.insert(out_red_filename.find_first_of("."), "_moment", 7);
        cv::imwrite(out_red_filename.c_str(), result[2]);
    }
    return 0;
}
