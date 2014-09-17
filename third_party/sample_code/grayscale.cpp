#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <opencv/cv.h>
#include <iostream>

using namespace cv;

int main( int argc, char** argv )
{
    char* imageName = argv[1];
    Mat image = imread( imageName, 1 );

    if( argc != 2 || !image.data ) {
        printf( " No image data \n " );
        return -1;
    }

    Mat gray_image;
    cvtColor( image, gray_image, CV_RGB2GRAY, 0 );

    imwrite( "Gray_Image.tif", gray_image );

    namedWindow( imageName, WINDOW_AUTOSIZE );
    namedWindow( "Gray image", WINDOW_AUTOSIZE );

    imshow( imageName, image );
    imshow( "Gray image", gray_image );
    waitKey(0);

    return 0;
}
