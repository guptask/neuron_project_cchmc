#Neuron Segmentation Project

##Packages to install

+ ###tiff library (not required currently) : 
>zip of the tiff library source files are present inside third\_party 
directory. Follow the README inside for details.

+ ###opencv: 
>Clone the opencv repo at https://github.com/Itseez/opencv. Follow the 
instructions for installation on Linux at http://opencv.org/. Add to 
~/.bashrc **export LD\_LIBRARY\_PATH=${LD\_LIBRARY\_PATH}:/usr/local/lib**. 
To test sample opencv code, compile using 
**g++ <file\_name> `pkg-config opencv --cflags --libs`**


##Build and run neuron segmentation package

Inside the project root directory, type **make** to build the project.
A binary called **segment** will be created.

Command to run the software: 
**./segment <image directory with / at end> <image list> <error file> 
<output csv file>**

