/*
 * Naive file compressor using miniz, a single C source file Deflate/Inflate compression 
 * library with zlib-compatible API (Project page: https://code.google.com/p/miniz/).
 *
 * To compile:
 *   g++ -std=c++11 -I$FF_ROOT -O3 simplecompressor.cpp -o simplecompressor
 */

#include "miniz.c"
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <iostream>
#include <ff/farm.hpp>
using namespace ff;

/* ----------------------- Utility functions ---------------------------- */
// helping functions: it parses the command line options
char* getOption(char **begin, char **end, const std::string &option) {
    char **itr = std::find(begin, end, option);
    if (itr != end && ++itr != end) return *itr;
    return nullptr;
}
// map the file pointed by filepath in memory
static inline bool mapFile(const std::string &filepath, unsigned char *&ptr, size_t &size) {
    // open input file.
    int fd = open(filepath.c_str(),O_RDONLY);
    if (fd<0) {
	printf("Failed opening file %s\n", filepath.c_str());
	return false;
    }
    struct stat s;
    if (fstat (fd, &s)) {
	printf("Failed to stat file %s\n", filepath.c_str());
	return false;
    }
    // checking for regular file type
    if (!S_ISREG(s.st_mode)) return false;
    // get the size of the file
    size = s.st_size;
    // map all the file in memory
    ptr = (unsigned char *) mmap (0, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (ptr == MAP_FAILED) {
	printf("Failed to map file %s in memory\n", filepath.c_str());
	return false;
    }
    close(fd);
    return true;
}
// unmap the file from memory
static inline void unmapFile(unsigned char *ptr, size_t size) {
    if (munmap(ptr, size)<0) {
	printf("Failed to unmap file\n");
    }
}
// write size bytes starting from ptr into a file pointed by filename
static inline void writeFile(const std::string &filename, unsigned char *ptr, size_t size) {
    FILE *pOutfile = fopen(filename.c_str(), "wb");
    if (!pOutfile) {
	printf("Failed opening output file %s!\n", filename.c_str());
	return;
    }
    if (fwrite(ptr, 1, size, pOutfile) != size) {
	printf("Failed writing to output file %s\n", filename.c_str());
	return;
    }
    fclose(pOutfile);
}

/* ---------------------------------------------------------------------- */

struct Task {
    Task(unsigned char *ptr, size_t size, const std::string &name):
        ptr(ptr),size(size),cmp_size(0),filename(name) {}

    unsigned char     *ptr;
    size_t             size;
    size_t             cmp_size;
    const std::string  filename;
};

// 1st stage
struct Read: ff_node {
    Read(char **filenames, const long num_files):
        filenames((const char**)filenames),num_files(num_files) {}

    void *svc(void *) {
	for(long i=0; i<num_files; ++i) {
	    const std::string &filepath(filenames[i]);
	    std::string filename;
	    
	    // get only the filename
	    int n=filepath.find_last_of("/");
	    if (n>0) filename = filepath.substr(n+1);
	    else     filename = filepath;
	
	    unsigned char *ptr = nullptr;
	    size_t size=0;
	    if (!mapFile(filepath, ptr, size)) continue;
	    
	    Task *t = new Task(ptr, size, filename);
	    ff_send_out(t);
        }        
        return EOS; // computation completed
    }

    const char **filenames;
    const long num_files;
};
// 2nd stage
Task* Compressor(Task *task, ff_node *const node) {
	unsigned char * inPtr = task->ptr;
	size_t          inSize= task->size;
	// get an estimation of the maximum compression size
	unsigned long cmp_len = compressBound(inSize);
	// allocate memory to store compressed data in memory
	unsigned char *ptrOut = new unsigned char[cmp_len];
	if (compress(ptrOut, &cmp_len, (const unsigned char *)inPtr, inSize) != Z_OK) {
	    printf("Failed to compress file in memory\n");	   
	    delete [] ptrOut;
	} else {
	    task->ptr      = ptrOut;
	    task->cmp_size = cmp_len;
	    node->ff_send_out(task);
	}
	unmapFile(inPtr, inSize);
	return reinterpret_cast<Task*>(GO_ON);
}
// 3rd stage
Task *Write(Task *task, ff_node*const) {
	const std::string outfile = "./out/" + task->filename + ".zip";
	// write the compressed data into disk 
	writeFile(outfile, task->ptr, task->cmp_size);
    	std::cout << "Compressed file " << task->filename << " from " << task->size << " to " << task->cmp_size << "\n";
	delete [] task->ptr;
	delete task;
	return reinterpret_cast<Task*>(GO_ON);
};

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "use: " << argv[0] << " [-n nworkers=2] file [file]\n";
        return -1;
    }

    int start = 1;    
    int nworkers = 2;
    char *n = getOption(argv, argv+argc, "-n");
    if (n) { nworkers = atoi(n); start+=2; argc-=2; }
    long num_files = argc-start;
    assert(num_files >= 1);

    Read farmEmitter(&argv[start], num_files); 
    ff_node_F<Task> farmCollector(Write); // build an ff_node from a function

    ff_Farm<> farm([nworkers] () {
	    std::vector<std::unique_ptr<ff_node> > Workers;
	    for(long i=0;i<nworkers;++i) 
		Workers.push_back(make_unique<ff_node_F<Task> >(Compressor));
	    return Workers;
	} (), farmEmitter, farmCollector);
    if (farm.run_and_wait_end()<0) {
        error("running farm\n");
        return -1;
    }        	
    return 0; 
}

