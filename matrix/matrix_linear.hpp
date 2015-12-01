#include <iostream>


using namespace std;
typedef unsigned long ul;

template <class T>
class Matrix{
  T *m;
  ul rows, cols;
  ul size;

public:
    class Row {
        friend class Matrix;
    public:
        T& operator[](ul col){
	  //cout << "row " << row <<"col " << col << endl;
	  //cout << parent.cols << endl;
          return parent.m[parent.cols*row + col];
        }
    private:
        Row(Matrix &parent_, ul row_) :
            parent(parent_),
            row(row_){}

        Matrix& parent;
        ul row;
    };

  Matrix(ul, ul);
  Matrix(ul, ul, T initValue);


  Row operator[](ul row){
      return Row(*this, row);
  }

};

template <class T>
Matrix<T>::Matrix(ul rows, ul cols, T initValue) : rows(rows), cols(cols), size(rows*cols){
  m = new T[rows*cols];
  for(ul i=0; i < this->size; ++i){
    m[i] = initValue;
  }
}
template <class T>
Matrix<T>::Matrix(ul rows, ul cols):Matrix(rows,cols,0){}
