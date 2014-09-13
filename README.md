
Packages to install
-------------------

1. tiff library: 
zip of the tiff library source files are present 
inside third\_party directory. Follow the README inside for details.

2. opencv: 
Clone the opencv repo at https://github.com/Itseez/opencv
Follow the instructions for installation on Linux at http://opencv.org/
Add to ~/.bashrc 'export LD\_LIBRARY\_PATH=${LD\_LIBRARY\_PATH}:/usr/local/lib'
Run sample code using 'g++ <file\_name> `pkg-config opencv --cflags --libs'

Inside the project root directory, type make to build the project.
A binary called 'segment' will be created.
