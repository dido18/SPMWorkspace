#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <sys/time.h>

#include <ff/parallel_for.hpp>

using namespace ff;
using ull = unsigned long long;

// see http://en.wikipedia.org/wiki/Primality_test
static bool is_prime(ull n) {
    if (n <= 3)  return n > 1; // 1 is not prime !
    
    if (n % 2 == 0 || n % 3 == 0) return false;

    for (ull i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) 
            return false;
    }
    return true;
}

int main(int argc, char *argv[]) {    
    if (argc<4) {
        std::cerr << "use: " << argv[0]  << " start end nWorkers [print=off|on]\n";
        return -1;
    }
    ull n1          = std::stoll(argv[1]);
    ull n2          = std::stoll(argv[2]);  
    size_t nWorkers = std::stoll(argv[3]);
    bool print_primes = false;

    if (argc >= 4)  {
      print_primes = (std::string(argv[4]) == "on");
    }

    ParallelFor pf;

    // vector of vectors, every worker write in different vector the prime number found
    std::vector<std::vector<ull>> results(nWorkers);


    // time t0,t1 needed to calculate the times spent for finding primes
    struct timeval t0,t1;
    gettimeofday(&t0,NULL);   

    //parallel_for search primes number in parallel
    pf.parallel_for_idx(n1,n2,1,0,[&results](const long begin, const long end, const int thid){
	// std::cout << "Hello I'm worker " << thid << " work on [" << begin <<"," << end<<") "<< std::endl;
        for( long i=begin; i< end; ++i)
           if(is_prime(i)){
	     (results.at(thid)).push_back(i);
            }
     },nWorkers);

    gettimeofday(&t1,NULL);
 

   // if argv[4]==on print the primes number found
    if (print_primes) {
      for( size_t i = 0; i< results.size(); ++i){
	for( size_t j = 0; j < results[i].size(); ++j){
	  if( i == results.size()-1 && j ==results[i].size()-1 ) 
	    std::cout << (results.at(i)).at(j)<<"\n";
	  else
	    std::cout << (results.at(i)).at(j)<<", " ;
	}
      }
    }

    int found =0;
    for( int j=0; j< results.size(); ++j){
      found+= results[j].size();
    }
    
    double elapsed = (t1.tv_sec-t0.tv_sec) + ((t1.tv_usec-t0.tv_usec)/1000000.0); 
    printf("Found %i primes in  %.6lf seconds \n ", found, elapsed); 
    return 0;
}


