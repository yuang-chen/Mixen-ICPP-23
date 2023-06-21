#pragma once

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <immintrin.h>
#include <assert.h>
#include <pthread.h>
#include <numa.h>

#include <unordered_map>
#include <algorithm>
#include <iomanip>
#include <numeric>
#include <string>
#include <vector>
#include <iostream>
#include <cmath>
#include <atomic>
#include <mutex>
#include <memory>
#include <deque>
#include <thread>
#include <pthread.h>

#include <boost/timer/timer.hpp>
#include "vec2d.hpp"

//#define WEIGHTED
#ifdef HUGE_V
using EdgeId = uint64_t;
#else
using VerxId = uint32_t;
#endif

#if defined (HUGE_E) || defined (HUGE_V)
using EdgeId = uint64_t;
#else
using EdgeId = uint32_t;
#endif


//typedef uint64_t EdgeId;
using Attr_t = float;
//typedef double Attr_t;
using bool_byte = uint8_t;

inline const std::string BoolToString(bool b)
{
  return b ? "true" : "false";
}

namespace params {
    extern unsigned subgraph_size;//(256*1024)/sizeof(float); //512kB cluster size is for cluster constructing
    extern unsigned subgraph_offset; 
    extern unsigned overflow_ceil;
    extern unsigned threads;
    extern unsigned iters;
    extern unsigned rounds;
    extern unsigned root_vertex;
    extern bool is_dynamic;
    extern bool is_filter;
    extern std::string input_file;
    extern std::string output_file;
};


namespace MASK {
    const unsigned MAX_NEG = 0x80000000;
    const unsigned MAX_POS = 0x7fffffff;
    const unsigned MAX_UINT = 0xffffffff;
    const unsigned MSB = 31;
};

struct Empty { };

template <typename EdgeData>
struct EdgeUnit {
  VerxId src;
  VerxId dst;
  EdgeData edge_data;
} __attribute__((packed));

template <>
struct EdgeUnit <Empty> {
  VerxId src;
  union {
    VerxId dst;
    Empty edge_data;
  };
} __attribute__((packed));
