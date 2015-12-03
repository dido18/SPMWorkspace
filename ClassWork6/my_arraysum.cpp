#include <iostream>
#include <ff/parallel_for.hpp>
using namespace ff;
const size_t SIZE= 5;//1<<20;

int main(int argc, char * argv[]) {    
  assert(argc > 1);
  int  nworkers  = atoi(argv[1]);
  // creates the array
  double *A = new double[SIZE];

  ParallelForReduce<double> pfr(nworkers);
  // fill out the array A using the parallel-for
  pfr.parallel_for(0,SIZE,1, 0, [&](const long j) { A[j]=j*1.0;});
  
  auto reduceF = [](double& partialSum, const double elem) {
    std::cout << "Reduce partial sum " << partialSum << std::endl;
    std::cout << "Elem reduce " << elem<< std::endl;
    partialSum += elem;
  };
  auto bodyF   = [&A](const long j, double& mySum) { 
    mySum += A[j];
    std::cout << "position"<< j << ": mySum " << mySum <<std::endl;
  };
  {
    double sum = 0.0;
    std::cout << "\nComputing sum with " << std::max(1,nworkers/2) << " workers, default static scheduling\n";	

    pfr.parallel_reduce(sum, 0.0, 0L, SIZE,
                        bodyF, reduceF, std::max(1,nworkers/2));
    std::cout << "Sum = " << sum << "\n\n";
  }
}
