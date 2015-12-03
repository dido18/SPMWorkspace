#include <vector>
#include <iostream>
#include <ff/pipeline.hpp>
#include <ff/map.hpp>

using namespace ff;

using Task = std::vector<float>;

const size_t columnsize = 1024;

struct Generator: ff_Map<Task> {
    using map = ff_Map<Task>;

    Generator(const size_t streamlen, const size_t maxsize):
	map(2),
	streamlen(streamlen),maxsize(maxsize) {}
    Task *svc(Task*) {	
      for(size_t i=0;i<streamlen; ++i) {

	    const size_t size = maxsize;

	    Task *t = new Task(size*columnsize);
	    Task &task = *t;
	    map::parallel_for(0,task.size(), 1, 0, 
			      [&](const long j) {
				  task[i] = i+j;
			      });
	    
	    ff_send_out(t);
	}
	return EOS;
    }

    const size_t streamlen;
    const size_t maxsize;
};

struct DP: ff_Map<Task,Task, float> {
    using map = ff_Map<Task,Task,float>;

    DP(std::vector<float> &initial_state,
       const size_t nworkers):map(nworkers), state(initial_state) {}

    Task *svc(Task *in) {
	Task &M = *in;
	

	float sum = 0.0;
	const size_t numrows = M.size()/columnsize;
	map::parallel_reduce(sum, 0.0,
			     0, numrows, 1, 0,
			     [&](const long i, float &sum) { 
				 const size_t idx = i*columnsize;
				 for(size_t j=0;j<columnsize;++j)
				     sum += M[idx + j]/state[j];
			     }, [](float &v, const float &elem) { v += elem;});


	map::parallel_for(0,columnsize,[&](const long i) { state[i] += sum; });
	
	delete in;
	return GO_ON;
    }

    void svc_end() {
	map::parallel_reduce(result, 0.0,
			     0,columnsize,[&](const long i, float &sum) { sum += state[i]; },
			     [](float &v, const float &elem) { v+=elem; });
    }

    std::vector<float> &state;
    float  result = {0};
};


int main(int argc, char *argv[]) {

    if (argc<4) {
	std::cerr << "use: " << argv[0]  << " streamlen max-array-size mapworkers \n";
	return -1;
    }
    const size_t streamlen  = std::stol(argv[1]);
    const size_t maxsize    = std::stol(argv[2]);
    const size_t mapworkers = std::stol(argv[3]);

    std::vector<float> state(columnsize);
    for(size_t i=0;i<columnsize;++i) state[i]=1.0;

    Generator gen(streamlen, maxsize);
    DP        map(state, mapworkers);
    ff_Pipe<> pipe(gen, map);
    ffTime(START_TIME);
    if (pipe.run_and_wait_end() <0){
	error("running pipe\n");
	return -1;
    }
    ffTime(STOP_TIME);
    std::cout << "Result: " << map.result << "\n";
    std::cout << "Time: "<< ffTime(GET_TIME) << " (ms)\n";

    return 0;
}
