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

/*  Parallel pipeline with a task-farm stage version of the 'img' program
 * 
 *    pipeline( Read, farm( pipeline( BlurFilter, EmbossFilter) ),  Write );
 *
 *  
 *                        ---> BlurFilter --> EmbossFilter --
 *                       |                                   |
 *    Read --> Scheduler ----> BlurFilter --> EmbossFilter -- -->Write
 *                       |                                   |
 *                        ---> BlurFilter --> EmbossFilter --
 *
 *  command: 
 *
 *    img_farm+pipe -r 1.5 -s 0.6 -n 2 `ls ./in\/*.gif`
 *
 *  the output is produced in the directory ./out
 *  with -n it is possible to set the task-farm workers (default 2)
 *
 */

/* 
 * Author: Massimo Torquati <torquati@di.unipi.it> 
 * Date:   August 2014
 */

#include <cassert>
#include <iostream>
#include <string>
#include <algorithm>

#include <ff/pipeline.hpp>
#include <ff/farm.hpp>

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

// 1st stage
struct Read: ff_node_t<Task> {
    Read(char **images, const long num_images, double r, double s):
        images((const char**)images),num_images(num_images),radius(r),sigma(s) {}

    Task *svc(Task *) {
        for(long i=0; i<num_images; ++i) {
            const std::string &filepath(images[i]);
            std::string filename;
            
            // get only the filename
            int n=filepath.find_last_of("/");
            if (n>0) filename = filepath.substr(n+1);
            else     filename = filepath;
            
            Image *img = new Image;;
            img->read(filepath);      
            Task *t = new Task(img, filename,radius,sigma);
            //std::cout << "sending out " << filename << "\n";
            ff_send_out(t); // sends the task t to the next stage
        }        
        return EOS; // computation completed
    }

    const char **images;
    const long num_images;
    const double radius;
    const double sigma;
};

// function executed by the 2nd stage
Task* BlurFilter(Task *in, ff_node*const) {
    //std::cout << "BlurFilter got  " << in->name << "\n";
    in->image->blur(in->radius, in->sigma);
    return in;

}
// function executed by the 3rd stage
Task* EmbossFilter(Task *in, ff_node*const) {
    //std::cout << "EmbossFilter got  " << in->name << "\n";
    in->image->emboss(in->radius, in->sigma);
    return in;
}
// function executed by the 4th stage
Task *Write(Task* in, ff_minode*const node) {
    //std::cout << "Wtite got  " << in->name << " from " << node->get_channel_id() << "\n";
    std::string outfile = "./out/" + in->name;
    in->image->write(outfile);
    std::cout << "image " << in->name << " has been written to disk\n";
    delete in->image;
    delete in;
    return (Task*)GO_ON;
}
// 4th stage
struct Writer: ff_minode_t<Task> { // this is a multi-input node
    Task *svc(Task *task) {
        return Write(task, this);
    }
};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "use: " << argv[0] << " [-r radius=1.0] [-s sigma=.5] [ -n Wrks=2] [ -m embossWrk=2] image-file [image-file]\n";
        return -1;
    }
    double radius=1.0,sigma=0.5;
    int Wrks = 2;
    int start = 1; 
    char *r = getOption(argv, argv+argc, "-r");
    char *s = getOption(argv, argv+argc, "-s");
    char *n = getOption(argv, argv+argc, "-n");
    if (r) { radius    = atof(r); start+=2; argc-=2; }
    if (s) { sigma     = atof(s); start+=2; argc-=2; }
    if (n) { Wrks      = atoi(n); start+=2; argc-=2; }

    InitializeMagick(*argv);
  
    long num_images = argc-1;
    assert(num_images >= 1);

    std::vector<std::unique_ptr<ff_node> > W;
    for(int i=0;i<Wrks;++i) {
#if 0
        std::unique_ptr<ff_node_F<Task> > blur(new ff_node_F<Task>(BlurFilter));
        std::unique_ptr<ff_node_F<Task> > emboss(new ff_node_F<Task>(EmbossFilter));     

        W.push_back(std::unique_ptr<ff_Pipe<Task> >(new ff_Pipe<Task>(blur, emboss)));
#endif
        W.push_back(make_unique<ff_Pipe<Task> >(make_unique<ff_node_F<Task> >(BlurFilter),
                                                make_unique<ff_node_F<Task> >(EmbossFilter)));
    }
    ff_Farm<Task> farm(std::move(W));
    farm.remove_collector();
    farm.set_scheduling_ondemand(); // set auto scheduling

    Read read(&argv[start], num_images, radius, sigma);  
    Writer write;
    ff_Pipe<> pipe(read,    // 1st stage
                   farm,    // 2nd stage
                   write);  // 3th stage
    if (pipe.run_and_wait_end()<0) { // executes the pipeline
        error("running pipeline\n");
        return -1;
    }

    return 0; 
}

