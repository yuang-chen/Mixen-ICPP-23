#pragma once

#include <boost/multi_array.hpp>
#include <boost/smart_ptr.hpp>

#include <vector>

template <typename T>
using array2d = typename boost::multi_array<T, 2>;

template<typename T>
using vec2d = std::vector<std::vector<T>>;

using bool_byte = uint8_t;


template<class T>
vec2d<T> initVec2d(unsigned row, unsigned col) {
    std::vector<std::vector<T>> vec2d (row);
    #pragma omp parallel for
    for(unsigned i = 0; i < row; i++) {
        vec2d[i] = std::vector<T>(col, 0);
    }
    return vec2d;
} 

template<class T>
T** allocArray2d(unsigned row, unsigned col) {
    T** array2d;
    array2d = new T* [row];
    for (unsigned i=0; i<row; i++)
        array2d[i] = new T [col]();
    return array2d;
}

template <class T>
void deleteArray2d (T** array2d, unsigned row)
{
    for (unsigned i=0; i<row; i++)
        delete[] array2d[i];
    delete[] array2d; 
}

// vec2d_bool getVec2dBool(unsigned row, unsigned col) {
//     vec2d_bool vec2d (row);
//     #pragma omp parallel for
//     for(unsigned i = 0; i < row; i++) {
//         vec2d[i] = std::deque<bool>(col);
//     }
//     return vec2d;
// }
/* unsigned get_segment_offset(unsigned index, std::vector<unsigned> offset) {
    unsigned rtn = 0;
    for(int i = 0; i < offset.size(); i++) {
        if(index < offset[i])
            break;
        else 
            rtn = i;
    }
    return rtn;
} */

template<class type>
class vector2d{
    const std::size_t rows, cols;
    std::vector<type> data;

    type &operator () (std::size_t x, std::size_t y) { 
        return data[x + y * cols]; 
    } 

    size_t size() const {
      return data.size();
   }   
};


template<class type>
class Array2d { 
  const std::size_t rows, cols, size; 
  //std::shared_ptr<type[]> data; // we do not need to implement all the contructors & destructor for smart pointers
    type* data;
 
public: 
    Array2d(std::size_t _rows, std::size_t _cols)
        : rows(_rows)
        , cols(_cols)
        , size(_rows * _cols)
        , data(new type[rows * cols]) {} 

    ~Array2d() {
      if(data != nullptr)   
        delete[] data;
    }
  // Copy constructor.
    Array2d(const Array2d & other)
        : rows(other.rows)
        , cols(other.cols)
        , size(other.size)
        , data(new type[rows * cols]) {   
        std::cout << "Array2d: copy constructor is called (inefficient!)" << '\n';
        std::copy(other.data, other.data + rows * cols, data);
    } 

 // copy assignment 
   Array2d& operator=(const Array2d& other) {
       std::cout << "Array2d: copy assignment is called (inefficient!)" << '\n';

      if (this != &other){
        // Free the existing resource.
        delete[] data;
        rows = other.rows;
        cols = other.cols;
        size = other.size;
        data = new type[size];
        std::copy(other.data, other.data + rows * cols, data);
      }
      return *this;
   }
// Move constructor
  Array2d(Array2d && other)
    : rows(0)
    , cols(0)
    , size(0)
    , data(nullptr) {
        rows = data.rows;
        cols = data.cols;
        size = data.size;
        data = other.data;
        // release the source object
        other.data = nullptr;
        other.rows = 0;
        other.cols = 0;
        other.size = 0;
    } 
// move assignment
    Array2d& operator=(Array2d&& other) {
        if (this != &other){
        // Free the existing resource.
        delete[] data;

        // Copy the data pointer and its length from the
        // source object.
        data = other.data;
        rows = other.rows;
        cols = other.cols;
        size = other.size;
        // Release the data pointer from the source object so that
        // the destructor does not free the memory multiple times.
        other.data = nullptr;
        other.rows = 0;
        other.cols = 0;   
        other.size = 0; 
        return *this;
     }
    }

    type &operator () (std::size_t x, std::size_t y) { 
        return data[x + y * cols]; 
    } 

    size_t get_size() const {
      return size;
   }   
}; 


int inline isNotZero(unsigned int n){ // unsigned is safer for bit operations
   return ((n | (~n + 1)) >> 31) & 1;
}

int inline isNotZero2(unsigned int n){ // return 0 or 2
   return (((n | (~n + 1)) >> 31) & 1) << 1;
}

template<class Iterator>
void checkRepeatition(const Iterator begin, const Iterator end) {
  auto size = std::distance(begin, end); //auto  = typename std::iterator_traits<Iterator>::difference_type 
  using type = typename std::iterator_traits<Iterator>::value_type;
  std::vector<type> dup(size);
  std::copy(begin, end, dup.begin());
  std::sort(dup.begin(), dup.end() );
  dup.erase(std::unique( dup.begin(), dup.end() ), dup.end() );
  std::cout<<"repeated elements: " << size - dup.size() <<'\n';
}