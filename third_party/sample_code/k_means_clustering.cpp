#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <opencv/cv.h>
#include <iostream>
#include <algorithm>

using namespace cv;


typedef struct {
    int red;
    int green;
    int blue;
    int groupNo;
} IMAGE;

typedef struct _MEANS { 
    int r, g, b;
    int groupNo;
} point_t, *point;

bool comp(int i, int j) { return i<j; }


int main (int argc, char *argv[]) {

    Mat img = imread(argv[1], 1);
    if (argc != 2 || !img.data) {
        std::cout <<  "Could not open or find the image" << std::endl ;
        return -1;
    }

    int clusterCount = 5; 
    IMAGE **strct = new IMAGE*[img.rows];
    for (int i = 0; i < img.rows; i++) {
        strct[i] = new IMAGE[img.cols];
        for (int j = 0; j < img.cols; j++) {
            strct[i][j].red = img.at<Vec3b>(i,j)[2];
            strct[i][j].green = img.at<Vec3b>(i,j)[1];
            strct[i][j].blue = img.at<Vec3b>(i,j)[0];
        }
    }

    int *tempuz = new int[clusterCount];
    point_t *p = new point_t[clusterCount];

    for (int k = 0; k < clusterCount; k++) {
        p[k].r = rand()%255;
        p[k].g = rand()%255;
        p[k].b = rand()%255;
        p[k].groupNo = k;
    }

    for (int i = 0; i < img.rows; i++) {
        for (int j = 0; j < img.cols; j++) {
            for (int k = 0; k < clusterCount; k++ ) {
                tempuz[k] = abs(strct[i][j].red   - p[k].r) + 
                            abs(strct[i][j].green - p[k].g) + 
                            abs(strct[i][j].blue  - p[k].b);  
            }
            strct[i][j].groupNo = *std::min_element(tempuz, tempuz+clusterCount, comp);
        }
    } 

    for (int i = 0; i < img.rows; i++) {
        for (int j = 0; j < img.cols; j++) {
            std::cout << strct[i][j].groupNo << ",";
        }
        std::cout << std::endl;
    }

    for (int steps = 0; steps < 200; steps++) {
        int r = 0, b = 0, g = 0, counter = 0;
        for (int k = 0; k < clusterCount; k++) { 
            for (int i = 0; i < img.rows; i++) {
                for (int j = 0; j < img.cols; j++) {
                    if(strct[i][j].groupNo == k) {
                        r += strct[i][j].red;
                        g += strct[i][j].green;
                        b += strct[i][j].blue;
                        counter++;
                    }
                }
            }
            if (counter != 0) {
                p[k].r = r/counter;
                p[k].g = g/counter;
                p[k].b = b/counter;
            }
        }
        steps++;

        for (int i = 0; i < img.rows; i++) {
            for (int j = 0; j < img.cols; j++) {
                for (int k = 0; k < clusterCount; k++) {
                    tempuz[k] = abs(strct[i][j].red   - p[k].r) +
                                abs(strct[i][j].green - p[k].g) +
                                abs(strct[i][j].blue  - p[k].b);  
                }
                strct[i][j].groupNo = *std::min_element(tempuz, tempuz+clusterCount, comp);
            }
        } 
    }

    for (int k=0; k < clusterCount; k++) { 
        for (int i = 0; i < img.rows; i++) {
            for (int j = 0; j < img.cols; j++) {
                if(strct[i][j].groupNo == k) {
                    strct[i][j].red   = p[k].r;
                    strct[i][j].green = p[k].g;
                    strct[i][j].blue  = p[k].b;
                }
            }
        }
    }

    for (int i = 0; i < img.rows; i++) {
        for (int j = 0; j < img.cols; j++) {
            img.at<Vec3b>(i,j)[2] = strct[i][j].red;
            img.at<Vec3b>(i,j)[1] = strct[i][j].green;
            img.at<Vec3b>(i,j)[0] = strct[i][j].blue;
        }
    }
    imwrite( "cluster.tif", img );
}
