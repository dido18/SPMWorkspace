#include <iostream>
#include <vector>
#include <ff/parallel_for.hpp>
#include <ff/pipeline.hpp>
#include <ff/farm.hpp>
#include <ff/map.hpp>

//#define  DEBUG  // decommnet for print the debugging

typedef unsigned long long ull;

/* my2_matrixvector_product.cpp
  this versio doesn't use the temp vecot array.
   inside the parallel_reduce operation olso the computatin of the multiplication T = A * S

Possible extension of  Classwork6
1) extra channel
2) shared memory
*/


using namespace ff;
using namespace std;

struct  myTask {
  myTask(int rows, int cols){
    matrix.resize(rows);
    for(int i =0; i <rows; ++i){
      matrix[i].resize(cols);
       for(int j=0; j <cols; ++j){
	 matrix.at(i).at(j) =i;
       }
    }
  }

  friend std::ostream& operator<<(std::ostream& out, const myTask& task){
    for(size_t i=0; i< task.matrix.size() ; ++i){
      for( size_t j=0; j<task.matrix.at(i).size(); ++j){
	if( j == task.matrix.at(i).size()-1)
	  out << task.matrix.at(i).at(j) << "\n";
	else
	  out << task.matrix.at(i).at(j);
      }
    }
    return out;
  }
 std::vector<std::vector<int>> matrix;
};

struct Firststage:ff_node_t<myTask>{

  Firststage(int numMatrixs, int nRows, int nCols):k(numMatrixs),rows(nRows),cols(nCols){}

  myTask *svc (myTask *){
    for(size_t i=0; i<k; ++i){
      myTask *matrix = new myTask(rows,cols);
#ifdef  DEBUG
      std::cout<< "Invio matrice:\n" << *matrix << std::endl;
#endif
      ff_send_out(matrix);
    }
    return EOS;
  }

  int k , rows, cols;
};

// ff_Map is just a ff_node that wraps a ParallelForReduce<T> pattern
struct mapWorker:ff_Map<myTask,void, ull>
{
   mapWorker(int nw):nWorkers(nw){}
   //alias
   using map = ff_Map<myTask,void, ull>;

    void *svc(myTask *task)
    {
          int num_rows = task->matrix.size();
	  //initialize internal state
          vstate.reserve(num_rows);
	  for(size_t i=0; i< num_rows; ++i)
	    vstate.push_back(1);

         ull sum = 0;

	 map::parallel_reduce(sum,0
			      ,0,num_rows
			      ,[&](const long idx, ull &mySum){
				            std::vector<int> &row = task->matrix.at(idx);
				            for(size_t j=0; j< row.size(); ++j)
				                mySum += row.at(j)*this->vstate[j];
			      },[&](ull& redSum, const int elem){
				            redSum += elem;
			      });

	 std::cout << "Sum reduced  " << sum << std::endl;

	 //update internal state S[i] += s
	 map::parallel_for(0,num_rows,[&](const long i) {
		  vstate[i] += sum;
	   },nWorkers);

     return GO_ON;
   }

  void  svc_end(){

    map::parallel_reduce(res,0,0,vstate.size(),
			 [&](const long idx, ull &myres){
			   myres += vstate.at(idx); 
			 },
			 [](ull &result, const int elem){ result += elem ;});
  }
   
    std::vector<int>  vstate; //internal state vector
    int nWorkers;
    ull res = {0};
};


int main(int argc, char *argv[]) {
    if (argc<5) {
      std::cout << "use: " << argv[0]  << " nrows ncols nmatrixes nWorkers\n";
        return -1;
    }
    int  n_rows      = std::stoll(argv[1]);
    int  n_cols      = std::stoll(argv[2]);
    int  n_mats       = std::stoll(argv[3]);
    int  n_workers   = std::stoll(argv[4]);

    Firststage emitMatrix(n_mats, n_rows, n_cols);
    mapWorker  mapW(n_workers);
    

    ff_Pipe<myTask> pipe(emitMatrix, mapW);

    ffTime(START_TIME);
    if(pipe.run_and_wait_end() <0) error("Runnning pipe");
    ffTime(STOP_TIME);

    std::cout << "Result: " <<   mapW.res << std::endl;
    std::cout << "Time: " <<   ffTime(GET_TIME) <<" (ms)" << std::endl;
    
    return (0);

}
