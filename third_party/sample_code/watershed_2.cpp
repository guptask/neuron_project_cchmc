#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

using namespace cv;
using namespace std;

int main (int argc, char * argv[]) {
    Mat image = imread(argv[1]);
    Mat binary;

    // Eliminate noise and smaller objects
    Mat fg;
    erode(binary,fg,Mat(),Point(-1,-1),2);

    // Identify image pixels without objects
    Mat bg;
    dilate(binary,bg,Mat(),Point(-1,-1),3);
    threshold(bg,bg,1,128,THRESH_BINARY_INV);

    // Create markers image
    Mat markers(binary.size(),CV_8U,Scalar(0));
    markers= fg+bg;

    markers.convertTo(markers, CV_32S);
    watershed(image,markers);

    markers.convertTo(markers,CV_8U);
    imshow("a",markers);
    waitKey(0);
}

