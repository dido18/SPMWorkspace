#include <iostream>
#include <vector>
#include <ff/parallel_for.hpp>
#include <ff/pipeline.hpp>
#include <ff/farm.hpp>
#include <ff/map.hpp>

//#define  DEBUG;  // decommnet for print the debugging

typedef unsigned long long ull;
/*
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
      std::cout<< "Invio matrice:\n  " << *matrix << std::endl;
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
	  //initialize temp vector
          std::vector<int> temp_vector(num_rows,0);

 	 // (T = A * S) A incoming matrix, S internal vector
	  map::parallel_for(0,num_rows,[&](const long idx){ //[&nWorkers, &task, &vstate,&temp_vector]
	    std::vector<int> row = task->matrix.at(idx);
	      for(int j=0; j<row.size(); ++j){
		temp_vector[idx] += row[j]*this->vstate[j];
	      }
	    },nWorkers);


	 auto bodyG = [&temp_vector](const long i,  ull& sum){
	   sum += temp_vector.at(i);
	 };
         auto reduceG = [](ull &sum, const int elem){ sum += elem;};

         ull sum = 0;

         //(sum T[i]) reduce phase on temp vector T
	 map::parallel_reduce(sum,0,0,num_rows,bodyG,reduceG,nWorkers); //var, identity, first, last, bodyF, reduceF,nworkers

#ifdef DEBUG
	 std::cout << "Sum reduced  " << sum << std::endl;
#endif
	 //S[i] += s
	 map::parallel_for(0,num_rows,[&](const long i) {
		  vstate[i] += sum;
	   },nWorkers);
	 //ff_send_out(vstate);
#ifdef DEBUG
	 std::cout << "Vector state updated\n";
	 for(int j=0; j<vstate.size(); ++j){
	   std::cout << vstate[j] << "  ";
	 }
#endif
	 return GO_ON;
    }

  void  svc_end(){


    auto bodyR = [&](const long i,  ull &r){
	   r += vstate.at(i);
    };

    auto reduceR = [](ull &res, const int elem){ res += elem;};

    ull res = 0;
    size_t size_state = vstate.size();

    //(res = sum T[i]) reduce phase on temp vector T
    map::parallel_reduce(res,0,0,size_state,bodyR,reduceR,nWorkers); //var, identity, first, last, bodyF, reduceF,nworkers
    std::cout << "Final result " << res << endl;

  }
    std::vector<int> vstate; //internal state vector
    int nWorkers;
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
    bool print_primes = false;

    if (argc >= 5)  {
      print_primes = (std::string(argv[5]) == "on");
    }

    Firststage emitMatrix(n_mats, n_rows, n_cols);
    mapWorker  mapW(n_workers);

    ff_Pipe<myTask> pipe(emitMatrix, mapW);

    if(pipe.run_and_wait_end() <0) error("Runnning pipe");
    return (0);

}
