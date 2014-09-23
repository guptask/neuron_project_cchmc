#include <unistd.h>
#include <memory>
#include <dirent.h>
#include <string.h>
#include <sstream>
#include <sys/stat.h>
#include "ColorChannelSeparator.hpp"
#include "CannyDetector.hpp"
#include "WatershedSegmentation.hpp"

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

    // Create the color channel separator
    std::unique_ptr<ColorChannelSeparator> col_ch_separator = 
            std::unique_ptr<ColorChannelSeparator>(new ColorChannelSeparator(false));

    // Create the canny edge detector
    std::unique_ptr<CannyDetector> canny_detect = 
            std::unique_ptr<CannyDetector>(new CannyDetector());

    // Create the watershed segmentor
    std::unique_ptr<WatershedSegmentation> watershed_segment = 
            std::unique_ptr<WatershedSegmentation>(new WatershedSegmentation());

    for (uint8_t z_index = 1; z_index <= z_count; z_index++) {

        // Create the input filename and blue stream output filename
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

        // Create the in and out tiff file streams
        TIFFErrorHandler error_handler = TIFFSetWarningHandler(NULL);
        TIFF *in = TIFFOpen(in_filename.c_str(), "r");
        if (!in) {
            TIFFError(TIFFFileName(in), "Could not open input image");
            return 0;
        }

        TIFF *out_red = TIFFOpen(out_red_filename.c_str(), "w");
        if (!out_red) {
            TIFFError(TIFFFileName(out_red), "Could not open red output image");
            return 0;
        }

        TIFF *out_green = TIFFOpen(out_green_filename.c_str(), "w");
        if (!out_green) {
            TIFFError(TIFFFileName(out_green), "Could not open green output image");
            return 0;
        }

        TIFF *out_blue = TIFFOpen(out_blue_filename.c_str(), "w");
        if (!out_blue) {
            TIFFError(TIFFFileName(out_blue), "Could not open blue output image");
            return 0;
        }
        TIFFSetWarningHandler(error_handler);

        // Extract the rgb streams for each input image
        col_ch_separator->printChannelImage(in, out_red, true, false, false);
        TIFFWriteDirectory(out_red);

        col_ch_separator->printChannelImage(in, out_green, false, true, false);
        TIFFWriteDirectory(out_green);

        col_ch_separator->printChannelImage(in, out_blue, false, false, true);
        TIFFWriteDirectory(out_blue);

        // Apply watershed segmentation
        std::string watershed_red_filename = out_red_filename;
        out_red_filename.insert (out_red_filename.find_first_of("."), "_watershed", 10);
        watershed_segment->segment (watershed_red_filename, out_red_filename);

        std::string watershed_green_filename = out_green_filename;
        out_green_filename.insert (out_green_filename.find_first_of("."), "_watershed", 10);
        watershed_segment->segment (watershed_green_filename, out_green_filename);

        std::string watershed_blue_filename = out_blue_filename;
        out_blue_filename.insert (out_blue_filename.find_first_of("."), "_watershed", 10);
        watershed_segment->segment (watershed_blue_filename, out_blue_filename);

        // Apply Canny Edge detection
        std::string canny_red_filename = out_red_filename;
        out_red_filename.insert (out_red_filename.find_first_of("."), "_canny", 6);
        canny_detect->detectEdges (canny_red_filename, out_red_filename);

        std::string canny_green_filename = out_green_filename;
        out_green_filename.insert (out_green_filename.find_first_of("."), "_canny", 6);
        canny_detect->detectEdges (canny_green_filename, out_green_filename);

        std::string canny_blue_filename = out_blue_filename;
        out_blue_filename.insert (out_blue_filename.find_first_of("."), "_canny", 6);
        canny_detect->detectEdges (canny_blue_filename, out_blue_filename);

        TIFFClose(in);
        TIFFClose(out_red);
        TIFFClose(out_green);
        TIFFClose(out_blue);
    }
    return 0;
}
