#include <iostream>
#include <string>
#include <opencv/cv.h>
#include "opencv2/opencv.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace cv;
//using namespace std;

class WatershedSegmenter {
public:
    void setMarkers (Mat& markerImage) {
        markerImage.convertTo(markers, CV_32S);
    }

    Mat process(Mat &image) {
        watershed(image, markers);
        markers.convertTo(markers,CV_8U);
        return markers;
    }

private:
    Mat markers;
};


int main (int argc, char* argv[]) {

    Mat image = imread(argv[1]);
    Mat binary;
    cvtColor(image, binary, CV_RGB2GRAY);
    threshold(binary, binary, 10, 255, THRESH_BINARY);

    imshow("originalimage", image);
    imshow("originalbinary", binary);

    // Eliminate noise and smaller objects
    Mat fg;
    erode(binary,fg,Mat(),Point(-1,-1),2);
    imshow("fg", fg);

    // Identify image pixels without objects
    Mat bg;
    dilate(binary,bg,Mat(),Point(-1,-1),3);
    threshold(bg,bg,1, 128,THRESH_BINARY_INV);
    imshow("bg", bg);

    // Create markers image
    Mat markers(binary.size(),CV_8U,Scalar(0));
    markers= fg+bg;
    imshow("markers", markers);

    // Create watershed segmentation object
    WatershedSegmenter segmenter;
    segmenter.setMarkers(markers);

    Mat result = segmenter.process(image);
    result.convertTo(result,CV_8U);
    imshow("final_result", result);

    waitKey(0);

    return 0;
}
