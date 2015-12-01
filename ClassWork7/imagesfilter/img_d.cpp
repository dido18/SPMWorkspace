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

/* 
 * This program is one possible distributed version of the img.cpp program:
 * 
 *  -------------------------------           -----------------------------------
 * |                     ------    |         |     --------                      |
 * |               ---->| Blur | --|---------|--> | Emboss |----                 |
 * |   --------   |     |      |   |         |    |        |    |   ----------   |  
 * |  | Read   |  |      ------    |         |     --------     |  | Collect. |  |
 * |  | +Sched |--|                |         |                  |--|  +Write  |  |
 * |   -------    |      ------    |         |     --------     |   ----------   |
 * |              |     | Blur |   |         |    | Emboss |    |                |
 * |               ---->|      | --|---------|--->|        |----                 |
 * |     farmBlur        ------    |         |     --------        farmEmboss    |
 * |                               |         |                                   |
 * |           Host  A             |         |               Host B              |
 *  -------------------------------           -----------------------------------
 *
 *  command: 
 *
 *
 *
 *  the output is produced in the directory ./out
 */

/* 
 * Author: Massimo Torquati <torquati@di.unipi.it> 
 * Date:   August 2014
 */

#include <sys/uio.h>
#include <ff/node.hpp>
#include <ff/svector.hpp>
#include <ff/dnode.hpp>
#include <ff/node.hpp>
#include <ff/farm.hpp>
#include <ff/pipeline.hpp>
#include <ff/utils.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <cstdio>
#include <Magick++.h> 

#include <ff/d/inter.hpp>
#include <ff/d/zmqTransport.hpp>
#include <ff/d/zmqImpl.hpp>


using namespace ff;
using namespace Magick;

#define COMM1  zmq1_1

template<class T> class Task {
private: 
    T * v;
    const std::string n;

    // needed for serialization/deserialization
    Blob * blob;
    svector<COMM1::TransportImpl::msg_t*>* msgs;
public:
    Task(T *v, const std::string &n):v(v),n(n),blob(nullptr),msgs(nullptr) {};
    ~Task() {
	if (v) {delete v; v=nullptr;}
	if (blob) {delete blob; blob=nullptr;}
	if (msgs) {
	    for(int i=0;i<msgs->size();++i)
            delete msgs->operator[](i);
	    delete msgs;
	    msgs=nullptr;
	}	
    }
    T* get() { return v; }
    const std::string& name() { return n; }
    
    Blob* getBlob() { if (!blob) blob=new Blob; return blob;}
    void setMsgBuffers(svector<COMM1::TransportImpl::msg_t*>* buf) { msgs = buf;}
};


// --------------------- Farm1 running on Host A -------------------
// this is the reader and scheduler
struct EmitterBlur: ff_node {
    EmitterBlur(std::vector<std::string> &imgs):imgs(imgs){};
  
    void * svc(void * t) {
        for(std::vector<std::string>::const_iterator it=imgs.begin(); 
            it!=imgs.end(); ++it) {

            const std::string &filepath(*it);
            std::string filename;
            
            // get only the filename
            int n=filepath.find_last_of("/");
            if (n>0) filename = filepath.substr(n+1);
            else     filename = filepath;

            Image * img = new Image();
            img->read(filepath);
            ff_send_out( new Task<Image>(img,filename));
        }
        return EOS;
    }    
    const std::vector<std::string> imgs;
}; 
// this is the worker of the Farm1
class WorkerBlur: public ff_dnode<COMM1> {
    typedef COMM1::TransportImpl        transport_t;
protected:
    static void callback(void *e, void* arg) {
        if (arg) {
            Task<Image> * task = (Task<Image> *) arg;
            delete task;
        }
    }
public:
    WorkerBlur(double rad, double sigma): rad(rad),sigma(sigma) {};
    WorkerBlur() { rad = 1.0; sigma=0.5;}
    WorkerBlur(const std::string &name, const std::string &address, transport_t *const transp):
        name(name),address(address),transp(transp) {
        rad = 1.0; sigma=0.5;
    }
    int svc_init() {
        ff_dnode<COMM1>::init(name, address, 1, transp, true, 0);  
        return 0;
    }   
    void * svc(void * t) {
        Task<Image> * task = (Task<Image> *) t;
        Image * img = task->get();
        img->blur(rad,sigma);
        return t;
    }
    void prepare(svector<iovec>& v, void* ptr, const int sender=-1) {
        Task<Image> * task = (Task<Image> *) ptr;
        Blob* blob = task->getBlob();
        task->get()->magick("BMP");
        task->get()->write(blob);
        struct iovec iov={const_cast<void*>(blob->data()),blob->length()};
        v.push_back(iov);
        setCallbackArg(ptr);
        const char * imagename = task->name().c_str();
        struct iovec iov2={const_cast<char*>(imagename),strlen(imagename)+1};
        v.push_back(iov2);
        setCallbackArg(nullptr);
    }    
private: 
    double rad, sigma;
protected:
    const std::string  name;
    const std::string  address;
    transport_t       *transp;
}; 

// --------------------- Farm2 running on Host B -------------------

class WorkerEmboss: public ff_dnode<COMM1> {
    typedef COMM1::TransportImpl        transport_t;
protected:
    static void callback(void *e, void* arg) {
        if (arg) {
            Task<Image> * task = (Task<Image> *) arg;
            delete task;
        }
    }
public:   
    WorkerEmboss(double rad, double sigma): rad(rad),sigma(sigma) {};    
    WorkerEmboss() { rad = 1.0; sigma=0.5;}   
    WorkerEmboss(const std::string &name, const std::string &address, transport_t *const transp):
        name(name),address(address),transp(transp) {
        rad = 1.0; sigma=0.5;
    }
    
    int svc_init() {
        ff_dnode<COMM1>::init(name, address, 1, transp, false, 0);  
        return 0;
    }
    
    void * svc(void * t) {
        Task<Image> * task = (Task<Image> *) t;
        Image * img = task->get();
        img->emboss(rad,sigma);
        return t;
    }

    void unmarshalling(svector<msg_t*>* const v[], const int vlen, void *& task) {
        assert(vlen==1 && v[0]->size()==2); 
        Blob blob(v[0]->operator[](0)->getData(), v[0]->operator[](0)->size());
        Image * I = new Image(blob);
        I->magick("GIF");
        std::string imagename((char*)(v[0]->operator[](1)->getData()));
        Task<Image> * t = new Task<Image>(I,imagename);
        t->setMsgBuffers(v[0]);
        task = t;
        // mettere dentro t il puntatore dei due svector per essere liberati 
    }
private: 
    double rad,sigma; 
protected:
    const std::string name;
    const std::string address;
    transport_t   * transp;
}; 


// this is the writer
struct  CollectorEmboss: ff_node {
    void * svc(void * t) {
        Task<Image> * task = reinterpret_cast<Task<Image> *>(t);
        Image * img = (task->get());
        std::string outfile = "./out/" + task->name();
        // img->magick("GIF");
        img->write(outfile);

        delete task;
        return GO_ON; 
    }
};

void usage(const char *name) {
    std::cerr << "\n\nuse:\n";
    std::cerr << " on hostA: " << name << " <0> <nw> <hostname> <startport> <image> [image]\n";
    std::cerr << " on hostB: " << name << " <1> <nw> <hostname> <startport>\n";
    std::cerr << " <-> required argument, [-] optional argument\n\n";
    std::cerr << " NOTE: TCP ports from startport to starport+nw must be opened on hostA and reacheble\n";
    std::cerr << " NOTE: in this implementation the n. of farm's workers must be the same!\n\n";
}

// --------------------------------------------------
int main(int argc, char *argv[]) {
    
    if (argc < 5) {
        usage(argv[0]);
        return -1;
    }
    
    int tag= atoi(argv[1]);   
    InitializeMagick(*argv);
    
    // creates the network using 0mq as transport layer
    zmqTransport transport(-1);
    if (transport.initTransport()<0) abort();
    
    int nw=-1, startport=-1;
    char *address=nullptr;
    std::vector<std::string> images;  

    switch(tag) {
    case 0: {          // HOST A
        if (argc<6) {
            usage(argv[0]);
            return -1;
        }
        nw        = atoi(argv[2]); 
        address   = argv[3];
        startport = atoi(argv[4]);
        for(int i=5; i<argc; i++) images.push_back(argv[i]);

        std::vector<ff_node *> wb;
        std::stringstream address1;
        for(int i=0; i<nw; i++)  {
            address1.str("");
            address1 << address << ":" << startport+i;
            wb.push_back(new WorkerBlur("A", address1.str(), &transport));
        }
        EmitterBlur E(images);
        ff_farm<> farmBlur(wb,&E);
        farmBlur.remove_collector();
        if (farmBlur.run_and_wait_end()<0) {
            error("running farmBlur\n");
            return -1;
        }
    } break;
    case 1: {          // HOST B
        if (argc != 5) {
            usage(argv[0]);
            return -1;
        }
        nw      = atoi(argv[2]); 
        address = argv[3];
        startport = atoi(argv[4]);
        std::stringstream address1;
        std::vector<ff_node *> we; 
        for(int i=0; i<nw; i++) {
            address1.str("");
            address1 << address << ":" << startport+i;
            we.push_back(new WorkerEmboss("A",address1.str(),&transport));
        }
        CollectorEmboss C;        
        ff_farm<> farmEmboss(we,nullptr,&C); 
        if (farmEmboss.run_and_wait_end()<0) {
            error("running farmEmboss\n");
            return -1;
        }
        printf("wTime= %f ms\n", farmEmboss.ffwTime());
        printf("Time= %f ms\n", farmEmboss.ffTime());
    } break;
    default: { usage(argv[0]); return -1;}
    }
    
    transport.closeTransport();
    return(0);   
}

