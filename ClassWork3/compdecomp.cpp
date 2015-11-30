// This is a modified version of the example3.c file distributed with miniz.c, it inflates/deflates 
// block by block. 
// For simplicity, this example is limited to files smaller than 4GB, but this is not a limitation of miniz.c.
#include "miniz.c"
#include <limits.h>
#include <unistd.h>
#include <algorithm>
#include <string>

const  size_t BUF_SIZE = (1024 * 1024);
static unsigned char s_inbuf[BUF_SIZE];
static unsigned char s_outbuf[BUF_SIZE];

static inline int compressFile(FILE *pInfile, size_t infile_size, FILE *pOutfile) {
    z_stream stream;
    // Init the z_stream
    memset(&stream, 0, sizeof(stream));
    stream.next_in = s_inbuf;
    stream.avail_in = 0;
    stream.next_out = s_outbuf;
    stream.avail_out = BUF_SIZE;
        
    size_t infile_remaining = infile_size;
    
    if (deflateInit(&stream, Z_BEST_COMPRESSION) != Z_OK) {
	printf("deflateInit() failed!\n");
	return -1;
    }    
    for ( ; ; ) {
	if (!stream.avail_in)  {
	    // Input buffer is empty, so read more bytes from input file.
	    size_t n = std::min(BUF_SIZE, infile_remaining);
	    if (fread(s_inbuf, 1, n, pInfile) != n) {
		printf("Failed reading from input file!\n");
		return -1;
	    }
	    stream.next_in    = s_inbuf;
	    stream.avail_in   = n;	  
	    infile_remaining -= n;
	}      
	int status = deflate(&stream, infile_remaining ? Z_NO_FLUSH : Z_FINISH);
	if ((status == Z_STREAM_END) || (!stream.avail_out))  {
	    // Output buffer is full, or compression is done, so write buffer to output file.
	    size_t n = BUF_SIZE - stream.avail_out;
	    if (fwrite(s_outbuf, 1, n, pOutfile) != n) {
		printf("Failed writing to output file!\n");
		return -1;
	    }
	    stream.next_out  = s_outbuf;
	    stream.avail_out = BUF_SIZE;
	}
	
	if (status == Z_STREAM_END)  break; // done
	else if (status != Z_OK) {
	    printf("deflate() failed with status %i!\n", status);
	    return -1;
	}
    } // for
    if (deflateEnd(&stream) != Z_OK) {
	printf("deflateEnd() failed!\n");
	return -1;
    }
    printf("Total input bytes: %u\n", (mz_uint32)stream.total_in);
    printf("Total output bytes: %u\n", (mz_uint32)stream.total_out);
    return 0;
}

static inline int decompressFile(FILE *pInfile, size_t infile_size, FILE *pOutfile) {
    z_stream stream;
    // Init the z_stream
    memset(&stream, 0, sizeof(stream));
    stream.next_in = s_inbuf;
    stream.avail_in = 0;
    stream.next_out = s_outbuf;
    stream.avail_out = BUF_SIZE;
    
    size_t infile_remaining = infile_size;
    if (inflateInit(&stream)) {
	printf("inflateInit() failed!\n");
	return -1;
    }
    
    for ( ; ; ) {
      if (!stream.avail_in)   {
	  // Input buffer is empty, so read more bytes from input file.
	  size_t n = std::min(BUF_SIZE, infile_remaining);
	  if (fread(s_inbuf, 1, n, pInfile) != n) {
	      printf("Failed reading from input file!\n");
	      return -1;
	  }
	  stream.next_in    = s_inbuf;
	  stream.avail_in   = n;	  
	  infile_remaining -= n;
      }
      int status = inflate(&stream, Z_SYNC_FLUSH);
      if ((status == Z_STREAM_END) || (!stream.avail_out)) {
	  // Output buffer is full, or decompression is done, so write buffer to output file.
	  size_t n = BUF_SIZE - stream.avail_out;
	  if (fwrite(s_outbuf, 1, n, pOutfile) != n) {
	      printf("Failed writing to output file!\n");
	      return -1;
	  }
	  stream.next_out = s_outbuf;
	  stream.avail_out = BUF_SIZE;
      }      
      if (status == Z_STREAM_END)  break; // done
      else if (status != Z_OK) {
	  printf("inflate() failed with status %i!\n", status);
	  return -1;
      }
    }// for
    if (inflateEnd(&stream) != Z_OK) {
	printf("inflateEnd() failed!\n");
	return -1;
    }
    printf("Total input bytes: %u\n", (mz_uint32)stream.total_in);
    printf("Total output bytes: %u\n", (mz_uint32)stream.total_out);
    return 0;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
      printf("Usage: %s c|d infile [outfile]\n",argv[0]);
      printf("\nModes:\n");
      printf("c - Compresses file infile to a zlib stream in file outfile\n");
      printf("d - Decompress zlib stream in file infile to file outfile\n");
      return -1;
  }
  const char *pMode = argv[1];
  if (!strchr("cCdD", pMode[0])) {
      printf("Invalid mode!\n");
      return -1;
  }
  const char *pSrc_filename = argv[2];
  char *pDst_filename = nullptr;
  std::string outfilename;
  if (argc >= 4) 
      pDst_filename = argv[3];
  else {
      const std::string infilename(pSrc_filename);
      if ((pMode[0] == 'c') || (pMode[0] == 'C')) { 
	  outfilename = infilename + ".zip";
	  pDst_filename = const_cast<char *>(outfilename.c_str());
      } else {
	  int n = infilename.find(".zip");
	  if (n>0) outfilename = infilename.substr(0,n);
	  else     outfilename = infilename + "_decomp";
	  pDst_filename = const_cast<char *>(outfilename.c_str());
      }
  }


  // Open input file.
  FILE *pInfile = fopen(pSrc_filename, "rb");
  if (!pInfile) {
    printf("Failed opening input file!\n");
    return -1;
  }

  // Determine input file's size.
  fseek(pInfile, 0, SEEK_END);
  long file_loc = ftell(pInfile);
  fseek(pInfile, 0, SEEK_SET);
  
  if ((file_loc < 0) || (file_loc > INT_MAX)) {
      // This is not a limitation of miniz or tinfl, but this example.
      printf("File is too large to be processed by this example.\n");
      return -1;
  }
  
  size_t infile_size = (size_t)file_loc;
  
  // Open output file.
  FILE *pOutfile = fopen(pDst_filename, "wb");
  if (!pOutfile) {
      printf("Failed opening output file!\n");
      return -1;
  }

  if ((pMode[0] == 'c') || (pMode[0] == 'C')) { // compression
      if (compressFile(pInfile, infile_size, pOutfile) <0) return -1;
  } else {                                      // decompression
      if (decompressFile(pInfile,infile_size, pOutfile) <0) return -1;
  }
  fclose(pInfile); fclose(pOutfile);
  if ((pMode[0] == 'd') || (pMode[0] == 'D')) unlink(pSrc_filename);
  printf("Success.\n");
  return 0;
}
