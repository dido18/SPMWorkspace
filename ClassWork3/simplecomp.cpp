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

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "use: " << argv[0] << " file [file]\n";
        return -1;
    }

    int start = 1;    
    long num_files = argc-start;
    assert(num_files >= 1);

    for(long i=0; i<num_files; ++i) {
        const std::string &filepath(argv[start+i]);
        std::string filename;
	
	unsigned char *ptr = nullptr;
	size_t size=0;
	if (!mapFile(filepath, ptr, size)) continue;

	// get an estimation of the maximum compression size
	unsigned long cmp_len = compressBound(size);
	// allocate memory to store compressed data in memory
	unsigned char *ptrOut = new unsigned char[cmp_len];
	if (compress(ptrOut, &cmp_len, (const unsigned char *)ptr, size) != Z_OK) {
	    printf("Failed to compress file in memory\n");
	    delete [] ptrOut;
	    unmapFile(ptr, size);
	    continue;
	}	   
	unmapFile(ptr, size);
	
        // get only the filename
        int n=filepath.find_last_of("/");
	if (n>0) filename = filepath.substr(n+1);
        else     filename = filepath;
	const std::string outfile = "./out/" + filename + ".zip";
	// write the compressed data into disk 
	writeFile(outfile, ptrOut, cmp_len);
	delete [] ptrOut;
	std::cout << "Compressed file " << filename << " from " << size << " to " << cmp_len << "\n";
    }
    return 0; 
}

