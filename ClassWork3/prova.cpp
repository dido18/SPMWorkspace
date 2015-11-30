
#include <iostream>

using namespace std;

struct Task{
  int m;
 #if 1
  Task(int a):m(a){
     cout << ":m(a) constructor"<<"\n";
  }
 #elif 0
  Task(int a):m{a}{
    cout << ":m{a} constructor"<<"\n";
}
 #else
  Task(int a){
  cout << "{int  m{a}} constructor"<<"\n";
   int m{a};
}
#endif
 
  Task(const Task& o) : m(o.m){
  cout << "you are copying me"<<endl;
  }

  Task( Task&& o) : m(o.m){
  cout << "you are moving me"<<endl;
  //delete &o; MEMORY LEAK 
}

  Task &operator=(const Task& o){
  cout << "you are assigning me"<<endl;
  // return Task(o.m);
  return *this;
}
};


int main(int argc, char *[]){
  // create new Objects
  //  Task a = Task{1.5}; //errore: conversion from long to int
  Task a = Task(1);
      Task b = 2.5; //solo quando prende un solo argomento
      Task c(3.5);
      Task *d = new Task(4.5);

  // MOVE objects
      Task e = std::move(a);
      Task h(std::move(b));
      
      
    Task f = b;         // copy constructor ???      
    Task g = Task(c);   // copy constructor


    std::cout << " a: " << a.m << "\n";  //1
    std::cout << " b: " << b.m << "\n";  //2
    std::cout << " c: " << c.m << "\n";  //3
    std::cout << " d: " << d->m << "\n"; //(*d).m = 4

    std::cout << " e: " << e.m << "\n"; // 1
    std::cout << " f: " << f.m << "\n";
    std::cout << " g: " << g.m << "\n";

  
  }
