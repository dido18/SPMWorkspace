
 // my_reduce_sum.cpp
#include <iostream>
#include <ff/parallel_for.hpp>

using namespace ff;
using namespace std;

size_t SIZE ;

int main( int argc,char *argv[]){

  if(argc< 3){
    cout<< "usage: \n" << argv[0] << " sizeVector nWorkers" << endl;
    return(0);
  }
  int nWorkers = std::stoll(argv[2]);

  SIZE = std::stoll(argv[1]);

  //create the array
  double *array = new double[SIZE];
 
  ParallelForReduce<double> pfr(nWorkers);
  
  auto f = [&array](const long i){ return array[i] = i;};

  //initialize array with parallel for
  pfr.parallel_for(0,SIZE,f,nWorkers);
  
  //sum with reduce operation
  ffTime(START_TIME);
  double sum =0.0;
  auto bodyF = [&array](const long j, double &sum){ sum += array[j];};
  auto reduceF = []( double &sum,const  double &elem){ sum +=elem;};
  pfr.parallel_reduce(sum,0.0,0,SIZE,bodyF,reduceF,nWorkers);
  ffTime(STOP_TIME);

  cout << "Final sum: " << sum <<"\nTime: "<< ffTime(GET_TIME) <<"(ms) "<< endl;
  return(0);

  
 
    
			   
  
  

}
