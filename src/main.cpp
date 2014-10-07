#include <iostream>
#include <unistd.h>
#include <memory>
#include <dirent.h>
#include <sys/stat.h>

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"

//#include "Moments.hpp"

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
    //std::unique_ptr<Moments> moments = std::unique_ptr<Moments>(new Moments());

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

        std::vector<cv::Mat> channels(3);
        cv::split(img, channels);

        // Enhance features
        //moments->apply(bgr[0], out_blue_filename);
        //moments->apply(bgr[1], out_green_filename);
        //moments->apply(bgr[2], out_red_filename);
    
        cv::imwrite(out_blue_filename.c_str(), channels[0]);
        cv::imwrite(out_green_filename.c_str(), channels[1]);
        cv::imwrite(out_red_filename.c_str(), channels[2]);
    }
    return 0;
}
