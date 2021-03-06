#include <stdlib.h>  //atoi
#include <iostream>

#include <ff/farm.hpp>
#include <ff/pipeline.hpp>

/*
Search the primes namber between [start,stop] range with nworkers 
using a farm skeleton with feedback. 
Each worker receive a number and return the task
if is a prime number to the Sched node.
The Sced node print the node.

usage:
   my_primes_mw  start stop nworkers
 */

using namespace ff;
struct Stage0: ff_node_t<long>{
  long start;
  long stop;
  Stage0(long s, long end): start(s),stop(end){}
  
  long *svc(long* ){
     for(long i=start; i<=stop; ++i)
      ff_send_out(new long(i));
     return EOS;
  }
};
struct Sched:ff_node_t<long>{
  int items;
  ff_loadbalancer *lb;
  Sched(ff_loadbalancer*const lb):lb(lb){}

  long *svc(long *t){
    int channel = lb->get_channel_id();
    if(channel ==-1){
      //std::cout << " Task " << *t <<" cooming from first stage0"<<"\n";
      return t;
    }
    std::cout << "PRIME Task: " <<*t<< " cooming by worker: "<< channel <<"\n";
    
    delete t;
    return GO_ON;
 
  }
  
  void eosnotify(ssize_t){
    //received EOS from stage0, broadcast EOS to ell the worker
    lb->broadcast_task(EOS);
  }
  
};

struct Worker:ff_node_t<long>{
  long *svc(long * task){
    //std::cout << "Worker "<< get_my_id()<<" check task: "<< *task<< "\n";
    if(this->is_prime(*task)){
     //td::cout << " worker No. "<< get_my_id()  <<" found prime "<< *task << "\n";
      return task;
    }
    return GO_ON;
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


/*
struct Endstage:ff_minode_t<long>{
  
  long *svc( long *task){
    std::cout<< "Collector " << get_my_id()<< " has received item: " << *task <<"\n";
    delete task;
    return GO_ON;
  }

};
*/

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
  
  Stage0  stage(start,stop);
  farm.remove_collector();
  Sched S(farm.getlb());
  farm.add_emitter(S); //add the Scheduler as emitter of farm
  farm.wrap_around(); // feddback channel  in the farm
  ff_Pipe<> pipe(stage,farm);

  if(pipe.run_and_wait_end()<0)error("runnning farm");
  return(0);
}
