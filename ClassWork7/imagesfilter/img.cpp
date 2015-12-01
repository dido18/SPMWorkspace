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

/*  This program computes the Blur and Emboss image filters on the input images and then
 *  save them into a separate disk directory using the same name.
 *
 *  command: 
 *
 *    img -r 1.5 -s 0.6 ./in/\*.gif
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

#include <Magick++.h> 
using namespace Magick;

// helping functions: it parses the command line options
char* getOption(char **begin, char **end, const std::string &option) {
    char **itr = std::find(begin, end, option);
    if (itr != end && ++itr != end) return *itr;
    return nullptr;
}

int main(int argc, char *argv[]) {
    double radius = 1.0; 
    double sigma  = 0.5;

    if (argc < 2) {
        std::cerr << "use: " << argv[0] << " [-r radius=1.0] [-s sigma=.5] image-files\n";
        return -1;
    }
    int start = 1;
    char *r = getOption(argv, argv+argc, "-r");
    char *s = getOption(argv, argv+argc, "-s");
    if (r) { radius = atof(r); start+=2; argc-=2; }
    if (s) { sigma  = atof(s); start+=2; argc-=2; }

    InitializeMagick(*argv);
    
    long num_images = argc-1;
    assert(num_images >= 1);
    // for each image apply the 2 filter in sequence    
    for(long i=0; i<num_images; ++i) {
        const std::string &filepath(argv[start+i]);
        std::string filename;
        
        // get only the filename
        int n=filepath.find_last_of("/");
        if (n>0) filename = filepath.substr(n+1);
        else     filename = filepath;
        
        Image img;
        img.read(filepath);
        
        img.blur(radius, sigma);
        img.emboss(radius, sigma);

        std::string outfile = "./out/" + filename;
        img.write(outfile);
        std::cout << "image " << filename << " has been written to disk\n";
    }
    return 0; 
}

