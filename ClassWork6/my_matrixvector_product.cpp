
#include <vector>
#include <ff/parallel_for.hpp>
#include <ff/pipeline.hpp>
#include <ff/farm.hpp>
#include <ff/map.hpp>
/*
Possible extension of  Classwork6
1) extra channel 
2) shared memory 
*/


using namespace ff;
using namespace std;

struct  myTask {
  myTask(int rows, int cols){
    matrix.reserve(rows);
    for(int i =0; i <rows; ++i){
       matrix[i].reserve(cols);
       for(int j=0; j <cols; ++j){
	 matrix.at(i).at(j) = i;
       }
    }    
  }

 std::vector<std::vector<int>> matrix;
  
};

struct Firststage:ff_node_t<myTask>{
  
  Firststage(int numMatrixs, int nRows, int nCols):k(numMatrixs),rows(nRows),cols(nCols){}
  
  myTask *svc (myTask *){
    for( int i =0; i<k; i++){
      myTask *matrix = new myTask(rows,cols);
      ff_send_out(&matrix);
    }   
    return EOS;
  }
 
  int k , rows, cols; 
};


//, std::vector<int>, int>
struct mapWorker:ff_Map<myTask,std::vector<int>, int>
{
    // ff_Map is just a ff_node that wraps a ParallelForReduce pattern 
   mapWorker(int nw):nWorkers(nw){}

    using map = ff_Map<myTask>;//alias

    std::vector<int> *svc( myTask *task)
    {
          int num_rows = task->matrix.size();
          vstate.reserve(num_rows);

          std::vector<int> temp_vector(num_rows);

	  /* auto bodyF = [&]( const long idx){ //&nWorkers, &task, &vstate,&temp_vector
	    std::vector<int> row = task->matrix.at(idx);
	      for(int j=0; j < row.size(); j++){
		temp_vector[idx] = row[j]*vstate[j];
	      }
	      };      
	  */

 	 // (T = A * S)matrix and internal vector product.
	  ff_Map<myTask>::parallel_for(0,num_rows,[&]( const long idx){ //&nWorkers, &task, &vstate,&temp_vector
	    std::vector<int> row = task->matrix.at(idx);
	      for(int j=0; j < row.size(); j++){
		temp_vector[idx] = row[j]*this->vstate[j];
	      }
	    },nWorkers);	      


	 auto bodyG = [&temp_vector](const long i,  int& sum){ sum += temp_vector[i];};     
         auto reduceG = [](int sum, const int elem){ sum+= elem;}; 
	   	      
         int sum = 0;
         //(sum T[i]) reduce phase on temp vector T
	 map::parallel_reduce(sum,0,0,num_rows,bodyG,reduceG,nWorkers); //var, identity, first, last, bodyF, reduceF,nworkers

         //S[i] += s
         map::parallel_for(0,num_rows,[&](const long i) {
		  vstate[i] += sum;
	 }
        ff_send_out(vstate);
    }
    std::vector<int> vstate; //internal state vector
    int nWorkers; 
};  


int main(int argc, char *argv[]) {    
    if (argc<5) {
        std::cerr << "use: " << argv[0]  << " nrows ncols nmatrixes nWorkers [print=off|on]\n";
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
    mapWorker  mapWorker(n_rows, n_workers);

    ff_Pipe<myTask> pipe(emitMatrix, mapWorker);

    if(pipe.run_and_wait() <0) error("Runnning pipe");
    return (0);
    
}
