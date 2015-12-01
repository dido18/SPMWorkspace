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

/*  Parallel pipeline version of the 'img' program (the pipeline is seen
 *  as a software accelerator):
 * 
 *     main
 *      |
 *    ->|
 *   |  Read
 *   |  offload --->  pipeline( BlurFilter, EmbossFilter, Write )
 *    - |
 *      |
 *
 *  command: 
 *
 *    img_pipe2 -r 1.5 -s 0.6 ./in\/*
 *
 *  the output is produced in the directory ./out
 */

/* 
 * Author: Massimo Torquati <torquati@di.unipi.it> 
 * Date:   August 2014
 */

#include <cassert>
#include <iostream>
#include <string>
#include <algorithm>

#define HAS_CXX11_VARIADIC_TEMPLATES 1  // needed to use the ff_pipe pattern
#include <ff/pipeline.hpp>

#include <Magick++.h> 
using namespace Magick;
using namespace ff;

struct Task {
    Task(Image *image, const std::string &name, double r=1.0,double s=0.5):
        image(image),name(name),radius(r),sigma(s) {};

    Image             *image;
    const std::string  name;
    const double       radius;
    const double       sigma;
};

char* getOption(char **begin, char **end, const std::string &option) {
    char **itr = std::find(begin, end, option);
    if (itr != end && ++itr != end) return *itr;
    return nullptr;
}

// function executed by the 2nd stage
Task* BlurFilter(Task *in, ff_node*const) {
    in->image->blur(in->radius, in->sigma);
    return in;
}
// function executed by the 3rd stage
Task* EmbossFilter(Task *in, ff_node*const) {
    in->image->emboss(in->radius, in->sigma);
    return in;
}
// function executed by the 4th stage
Task* Write(Task* in, ff_node*const) {
    std::string outfile = "./out/" + in->name;
    in->image->write(outfile);
    std::cout << "image " << in->name << " has been written to disk\n";
    delete in->image;
    delete in;
    return (Task*)GO_ON;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "use: " << argv[0] << " [-r radius=1.0] [-s sigma=.5] <image-file> [image-file]\n";
        return -1;
    }
    double radius=1.0,sigma=0.5;
    int start = 1; 
    char *r = getOption(argv, argv+argc, "-r");
    char *s = getOption(argv, argv+argc, "-s");
    if (r) { radius = atof(r); start+=2; argc-=2; }
    if (s) { sigma  = atof(s); start+=2; argc-=2; }

    InitializeMagick(*argv);
  
    long num_images = argc-1;
    assert(num_images >= 1);
        
    ff_node_F<Task> blur(BlurFilter);      
    ff_node_F<Task> emboss(EmbossFilter);  
    ff_node_F<Task> write(Write);          

    ff_Pipe<> pipe(true,            // enable accelerator
                   blur,            // 2nd stage
                   emboss,          // 3rd stage
                   write);          // 4th stage

    if (pipe.run_then_freeze()<0) { // start the pipeline
        error("running pipeline\n");
        return -1;
    }        
    for(long i=0; i<num_images; ++i) {
        const std::string &filepath(argv[start+i]);
        std::string filename;
        
        // get only the filename
        int n=filepath.find_last_of("/");
        if (n>0) filename = filepath.substr(n+1);
        else     filename = filepath;
        
        Image *img = new Image;;
        img->read(filepath);      
        Task *t = new Task(img, filename,radius,sigma);
        pipe.offload(t); // sends the task t to the pipeline
    }        
    pipe.offload(EOS); // End-Of-Stream

    if (pipe.wait()<0) { // wait for pipeline termination
        error("waiting pipeline\n");
        return -1;
    }        
    return 0; 
}

