#include <iostream>
#include <ff/pipeline.hpp>
#include <ff/farm.hpp>
using namespace ff;
using namespace std;

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


struct Worker:ff_node_t<myTask>{
    int svc_init(){
        std::cout <<" I'm the worker "<< get_my_id()<<std::endl;
    }
        myTask *svc(myTask *task){

           myTask &res = *task;
           res = res*res;
           return task; 
        }
};

struct ThirdStage:ff_minode_t<myTask>{ //multiple input channel node
       
    myTask *svc(myTask *task){
        std::cout << "Last stage, received " << *task<< " from " <<get_channel_id()<<std::endl;
        myTask& t = *task;
         sum+= t;
         delete task;
         return GO_ON;
        }
    void svc_end(){
        std::cout<< " sum = " <<sum<<std::endl;
    }
    myTask sum = 0.0;
 }Collector;


int main(int argc, char *argv[]){
    if(argc<3){
        cout<<"usage: \n my_farm_square nworkers nitems"<<endl;
        return(0);
    }
    int nworkers = atoi(argv[1]);
    FirstStage f1(std::stol(argv[2]));
    ThirdStage f3;
    
    ff_Farm<myTask> farm([nworkers](){
            std::vector<std::unique_ptr<ff_node>> workers;
            for( int i=0;i<nworkers;++i)
                workers.push_back(std::unique_ptr<ff_node_t<myTask>> (new Worker));
            return workers;
        }());
    farm.remove_collector();

    ff_Pipe<myTask> pipe(f1,farm,f3);
    ffTime(START_TIME);
    if(pipe.run_and_wait_end() <0) error(" runnnig Pipe");
    ffTime(STOP_TIME);
    std::cout <<"Time : " <<ffTime(GET_TIME)<<std::endl;
    return(0);
}
        
