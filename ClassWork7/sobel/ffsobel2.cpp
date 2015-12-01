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
 *    farm( ReadFileName+Sched, Map(Read+Sobel) )
 *  
 *  command: 
 *
 *    ffsobel  file1 file2 file3 ....
 *
 *  the output is produced in the directory ./out
 *
 *  This version does not display the images.
 */
/* 
 * Author: Massimo Torquati <torquati@di.unipi.it> 
 * Date:   August 2014
 */
#include <cassert>
#include <iostream>
#include <string>
#include <cmath>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
 
#include <ff/farm.hpp>
#include <ff/map.hpp>

using namespace ff;
using namespace cv;

struct Task {
    Task(const std::string &name): name(name) {}
    const std::string  name;
};

/* ----- utility function ------- */
template<typename T>
T *Mat2uchar(cv::Mat &in) {
    T *out = new T[in.rows * in.cols];
    for (int i = 0; i < in.rows; ++i)
        for (int j = 0; j < in.cols; ++j)
            out[i * (in.cols) + j] = in.at<T>(i, j);
    return out;
}
char* getOption(char **begin, char **end, const std::string &option) {
    char **itr = std::find(begin, end, option);
    if (itr != end && ++itr != end) return *itr;
    return nullptr;
}
#define XY2I(Y,X,COLS) (((Y) * (COLS)) + (X))
/* ------------------------------ */
// returns the gradient in the x direction
static inline long xGradient(uchar * image, long cols, long x, long y) {
    return image[XY2I(y-1, x-1, cols)] +
        2*image[XY2I(y, x-1, cols)] +
        image[XY2I(y+1, x-1, cols)] -
        image[XY2I(y-1, x+1, cols)] -
        2*image[XY2I(y, x+1, cols)] -
        image[XY2I(y+1, x+1, cols)];
}

// returns the gradient in the y direction 
static inline long yGradient(uchar * image, long cols, long x, long y) {
    return image[XY2I(y-1, x-1, cols)] +
        2*image[XY2I(y-1, x, cols)] +
        image[XY2I(y-1, x+1, cols)] -
        image[XY2I(y+1, x-1, cols)] -
        2*image[XY2I(y+1, x, cols)] -
        image[XY2I(y+1, x+1, cols)];
}

struct Read: ff_node_t<Task> {
    Read(char **images, const long num_images):
        images((const char**)images),num_images(num_images) {}
    
    Task *svc(Task *) {
        for(long i=0; i<num_images; ++i) {
            Task *t = new Task(images[i]);
            ff_send_out(t); // sends the task t to the next stage
        }        
        return EOS; // computation completed
    }
    const char **images;
    const long num_images;
};
struct SobelStage: ff_Map<Task> {
    // sets the maximum n. of worker for the Map
    // spinWait is set to true
    // the scheduler is disabled by default
    SobelStage(int mapworkers)
        :ff_Map<Task>(mapworkers,true) {}
    
    Task *svc(Task *task) {
        const std::string &filepath(task->name);
        std::string filename;    
        // get only the filename
        int n=filepath.find_last_of("/");
        if (n>0) filename = filepath.substr(n+1);
        else     filename = filepath;
        
        Mat src;
        src = imread(filepath, CV_LOAD_IMAGE_GRAYSCALE);
        if ( !src.data ) {
            error("reading image file %s, going on....\n", 
                  filepath.c_str());
            return GO_ON;
        }
        std::cout << "Image " <<  src.rows << " rows, " << src.cols << " cols\n";
        uchar * dst = new uchar[src.rows * src.cols]; 
        const long cols =  src.cols;
        ff_Map<Task>::parallel_for(0,src.rows,[cols,&dst](const long y) {
                for(long x = 0; x < cols; x++) 
                    dst[y * cols+ x] = 0;
            });
        uchar *src_uchar = Mat2uchar<uchar>(src);
        ff_Map<Task>::parallel_for(1,src.rows-1,[src_uchar,cols,&dst](const long y) {
                for(long x = 1; x < cols - 1; x++){
                    const long gx = xGradient(src_uchar, cols, x, y);
                    const long gy = yGradient(src_uchar, cols, x, y);
                    // approximation of sqrt(gx*gx+gy*gy)
                    long sum = abs(gx) + abs(gy); 
                    if (sum > 255) sum = 255;
                    else if (sum < 0) sum = 0;
                    dst[y*cols+x] = sum;
                }
            });    
        const std::string &outfile = "./out/" + filename;
        imwrite(outfile, cv::Mat(src.rows, src.cols, CV_8U, dst, cv::Mat::AUTO_STEP));
        delete task; delete [] dst;
        return GO_ON;
    };
};

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "\nuse: " << argv[0] 
                  << " [-n <Wrks1=2>] [-m <Wrks2=2>] <image-file> [image-file]\n";
        std::cerr << "   Wrks1 is the n. of farm's workers\n";
        std::cerr << "   Wrks2 is the n. of map's workers\n\n";
        return -1;
    }    
    int start = 1;
    int Wrks1 = 2;
    int Wrks2 = 2;
    char *n = getOption(argv, argv+argc, "-n");
    char *m = getOption(argv, argv+argc, "-m");
    if (n) { Wrks1  = atoi(n); start+=2; argc-=2; }
    if (m) { Wrks2  = atoi(m); start+=2; argc-=2; }

    long num_images = argc-1;
    assert(num_images >= 1);

    Read read(&argv[start], num_images);
    ff_Farm<> farm([Wrks1, Wrks2] () {
            std::vector<std::unique_ptr<ff_node> > W;
            for(int i=0;i<Wrks1;++i) 
                W.push_back(make_unique<SobelStage>(Wrks2));
            return W;
        } () , read);
    //farm.set_scheduling_ondemand();
    farm.remove_collector();           // ff_Farm has the collector by default !
    if (farm.run_and_wait_end()<0) {
        error("running farm\n");
        return -1;
    }        
    return 0;  
}
