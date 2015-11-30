#include <iostream>
#include <ff/pipeline.hpp>
#include <math.h>

#define ENABLE_F2 0

using namespace ff;
typedef long myTask;


struct FirstStage:ff_node_t<myTask>{ 
    
  FirstStage(int nitems):n(nitems){}

    myTask *svc(myTask*){
        for(int i=0; i<n ;i++){
            ff_send_out(new myTask(i));
        }
        return EOS;
    }  
   int n;
};

#if !ENABLE_F2
struct SecondStage:ff_node_t<myTask>{
        myTask *svc(myTask *task){
           myTask &res = *task;
	   res = res*res;
	   return task; 
        }
};
#else
myTask* F2(myTask *task, ff_node*const node){
  myTask &res = *task;
  res = res*res;
  return task;
}
#endif



struct ThirdStage:ff_node_t<myTask>{
       
    myTask *svc(myTask *task){
        myTask& t = *task;
         sum+= t;
         delete task;
         return GO_ON;
        }
    void svc_end(){
        std::cout<< " sum = " <<sum<<std::endl;
    }
    

    myTask sum = 0.0;
 };


int main(int argc, char *argv[]){

    FirstStage f1(std::stol(argv[1]));
    ThirdStage f3;
#if !ENABLE_F2
    SecondStage f2;
    ff_Pipe<myTask> pipe(f1,f2,f3);
#else
    ff_node_F<myTask> second(F2);
    ff_Pipe<myTask> pipe(f1,second,f3);
#endif
    if(pipe.run_and_wait_end() <0) error(" runnnig Pipe");
    return(0);
}
        
