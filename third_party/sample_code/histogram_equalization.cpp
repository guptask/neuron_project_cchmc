#include <iostream>
#include <opencv/cv.h>
#include "opencv2/opencv.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace cv;
using namespace std;

/**  @function main */
int main( int argc, char** argv ) {

    Mat src, dst;

    string source_window("Source image");
    string equalized_window("Equalized Image");

    /// Load image
    src = imread( argv[1], 1 );

    if( !src.data ) {
        cout<<"Usage: ./Histogram_Demo <path_to_image>"<<endl;
        return -1;
    }

    /// Convert to grayscale
    cvtColor( src, src, CV_BGR2GRAY );

    /// Apply Histogram Equalization
    equalizeHist( src, dst );

    /// Display results
    namedWindow( source_window, 1);
    namedWindow( equalized_window, 1);

    imshow( source_window, src );
    imshow( equalized_window, dst );

    /// Wait until user exits the program
    waitKey(0);

    return 0;
}
