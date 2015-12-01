/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ***************************************************************************
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as 
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  As a special exception, you may use this file as part of a free software
 *  library without restriction.  Specifically, if other files instantiate
 *  templates or use macros or inline functions from this file, or you compile
 *  this file and link it with other files to produce an executable, this
 *  file does not by itself cause the resulting executable to be covered by
 *  the GNU General Public License.  This exception does not however
 *  invalidate any other reasons why the executable file might be covered by
 *  the GNU General Public License.
 *
 ****************************************************************************
 */

/*  Computes the Sobel filter on each input image. For information concerning 
 *  the Sobel filter please refer to :
 *   http://en.wikipedia.org/wiki/Sobel_operator
 * 
 *  
 *  command: 
 *
 *    ffsobel  file1 file2 file3 ....
 *
 *  the output is produced in the directory ./out
 *
 */
/* 
 * Author: Massimo Torquati <torquati@di.unipi.it> 
 * Date:   August 2014
 */
#include<iostream>
#include<cmath>
#include<opencv2/imgproc/imgproc.hpp>
#include<opencv2/highgui/highgui.hpp>

using namespace cv;

struct Task {
    Task(Mat *src, Mat *dst, const std::string &name):
	src(src),dst(dst),name(name) {};
    Mat       *src, *dst;
    const std::string  name;
};

// returns the gradient in the x direction
static inline long xGradient(Mat image, long x, long y) {
    return image.at<uchar>(y-1, x-1) +
	2*image.at<uchar>(y, x-1) +
	image.at<uchar>(y+1, x-1) -
	image.at<uchar>(y-1, x+1) -
	2*image.at<uchar>(y, x+1) -
	image.at<uchar>(y+1, x+1);
}

// returns the gradient in the y direction 
static inline long yGradient(Mat image, long x, long y) {
    return image.at<uchar>(y-1, x-1) +
	2*image.at<uchar>(y-1, x) +
	image.at<uchar>(y-1, x+1) -
	image.at<uchar>(y+1, x-1) -
	2*image.at<uchar>(y+1, x) -
	image.at<uchar>(y+1, x+1);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "use: " << argv[0] 
		  << " <image-file> [image-file]\n";
        return -1;
    }    

    int start = 1;
    long num_images = argc-1;
    assert(num_images >= 1);

    Mat src, dst;
    for(long i=0; i<num_images; ++i) {
        const std::string &filepath(argv[i+start]);
        std::string filename;
        
        // get only the filename
        int n=filepath.find_last_of("/");
        if (n>0) filename = filepath.substr(n+1);
        else     filename = filepath;
        

        src = imread(filepath, CV_LOAD_IMAGE_GRAYSCALE);
        if ( !src.data ) {
            std::cerr << "ERROR reading image file " << filepath << " going on....\n";
            continue;
        }
        dst = src.clone();

        for(long y = 0; y < src.rows; y++)
            for(long x = 0; x < src.cols; x++)
                dst.at<uchar>(y,x) = 0.0;
        
        for(long y=1;y<src.rows-1;++y) {
            for(long x = 1; x < src.cols - 1; x++){
                const long gx = xGradient(src, x, y);
                const long gy = yGradient(src, x, y);
                // approximation of sqrt(gx*gx+gy*gy)
                long sum = abs(gx) + abs(gy); 
                if (sum > 255) sum = 255;
                else if (sum < 0) sum = 0;
                //dst.at<uchar>(y,x) = sum;
            }
        }

        const std::string &outfile = "./out/" + filename;
        imwrite(outfile, dst);
    }            
    return 0;  
}
