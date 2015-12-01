#include <iostream>
#include <functional>
#include "matrix_linear.hpp"

using namespace std;

int main (){

  Matrix<function<string()>> m(5,5,[](){return "prova!";});
  
  for(size_t i=0; i < 5; ++i){
    for(size_t j=0; j < 5; ++j){
      std::cout << m[i][j]() << std::endl;
      }
  }
  
  return(0);
}
