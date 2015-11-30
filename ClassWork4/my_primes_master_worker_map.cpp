#include <stdlib.h>  //atoi
#include <iostream>

#include <ff/farm.hpp>
#include <ff/pipeline.hpp>


using namespace ff;
struct myTask {
  myTask( long s, long e): start(s),stop(e){}
  long start;
  long stop;
  std::vector<long> results;
};

struct Sched:ff_node_t<myTask>{
  long start;
  long nWorkers;
  long delta;
  long stop;
  ff_loadbalancer *lb;
  std::vector<long> vPrimes;

  Sched(ff_loadbalancer*const lb,
	const long start,
	const long stop, 
	const long nw)
        :lb(lb), start(start), stop(stop),nWorkers(nw){
    delta = (stop-start)/nw;
  }

  myTask *svc(myTask * task){
    int channel = lb->get_channel_id();
    if(task==nullptr){ //first time the  Scheduler is called
      long tempStart= this->start;
      for(long i=0; i<this->nWorkers; ++i){
	if( i==(this->nWorkers-1)) //last worker take the remaining tasks
	  //lb->ff_send_out_to(new myTask(i,i+stop))-
	  ff_send_out(new myTask(tempStart,stop));
	else
	  //lb->ff_send_out_to(  
	  ff_send_out(new myTask(tempStart,tempStart+delta));
	tempStart += delta;
      }
      // broadcasting the End-Of-Stream to all workers
      lb->broadcast_task(EOS);
      // keep me alive 
      return GO_ON;
    } 
    // receives long prime from workers throgh  feddback channels,co
    //std::cout << "EMITTER: primes range cooming by worker: "<< channel <<"\n";
    auto &V = task->results;

    if(V.size()){// if is not empty the vector result from worker
      vPrimes.insert(std::upper_bound(vPrimes.begin(),vPrimes.end(),V[0]),V.begin(),V.end());
    }
    delete task;
    return GO_ON;
  }

};

struct Worker:ff_node_t<myTask>{
  myTask *svc(myTask *task){
    std::cout << "Worker "<< get_my_id()<<": checks range :["<< task->start<< ", "<< task->stop << "] \n";
    auto &vRes = task->results;
    for(long i=(task->start); i < (task->stop); ++i){
      if(this->is_prime(i)){
	//std::cout << "WORKER "<< get_my_id()  <<": found prime "<< i << "\n";
	vRes.push_back(i);
      }
    }
    return task;
  }

  // see http://en.wikipedia.org/wiki/Primality_test
  bool is_prime(long n) {
    if (n <= 3)  return n > 1; // 1 is not prime !
    
    if (n % 2 == 0 || n % 3 == 0) return false;

    for (long i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) 
            return false;
    }
    return true;
  }

};



int main(int argc, char *argv[]){

  if(argc <3){
    std::cout <<"usage: \n "<< argv[0]<<" start stop  nWorkers"<<"\n";
    return(0);
  }
  long start = atoi(argv[1]);
  long stop = atoi(argv[2]);
  int nWorkers = atoi(argv[3]);
  
  ff_Farm<long> farm([nWorkers]{
      std::vector<std::unique_ptr<ff_node> > Workers;
      for(int i=0; i<nWorkers; ++i)
	Workers.push_back(make_unique<Worker>());
      return Workers;
    }());//, emitter);//, collector );
  

  Sched S(farm.getlb(), start, stop, nWorkers);
  farm.add_emitter(S);      //add the Scheduler as emitter of farm
  farm.remove_collector();  // removes the default collector
  farm.wrap_around();       // feddback channel  in the farm
 

  if(farm.run_and_wait_end()<0)error("runnning farm");


  const std::vector<long> &res = S.vPrimes;
  const size_t n = res.size();
  std::cout << "Found "<< n <<" primes numbers"<<std::endl;
  for(size_t i=0; i<n; ++i){
    std::cout << res[i] <<" "; 
  }
  std::cout << std::endl;
  return(0);

}
