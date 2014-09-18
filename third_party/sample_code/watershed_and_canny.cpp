#include "opencv/cv.h"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

using namespace cv;
using namespace std;

int main (int argc, char * argv[]) {

    Mat im = imread(argv[1]);
    Mat gray;
    cvtColor(im, gray, CV_BGR2GRAY);

    // Use Canny instead of threshold to catch squares with gradient shading
    Mat bw;
    Canny(gray, bw, 0, 100, 5, true);

    // Find contours
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    findContours( bw, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE );

    // watershed
    Mat markers(bw.size(), CV_32S);
    markers = Scalar::all(0);
    int idx = 0;
    int compCount = 0;
    for( ; idx >= 0; idx = hierarchy[idx][0], compCount++ ) {
        if (fabs(contourArea(contours[compCount])) < min_size ) {
            continue;
        }
        drawContours(markers, contours, idx, Scalar::all(compCount+1), 1, 8, hierarchy, INT_MAX);
    }
    watershed( im, markers );

    return 0;
}

