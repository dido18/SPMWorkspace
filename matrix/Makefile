CXX		= g++ -std=c++11 -g -Wall

TARGETS		= matrix_linear main_matric

.PHONY: all clean cleanall


%: %.hpp
	$(CXX) $^ -o $@  

%: %.cpp
	$(CXX) $^ -o $@

all		: $(TARGETS)
clean		: 
	rm -f $(TARGETS)
cleanall	: clean
	\rm -f *.o *~



