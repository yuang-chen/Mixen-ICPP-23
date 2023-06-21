#include "global.hpp"

// template<class Graph_Func, class Graph, class Subgraph>
// void run(Graph graph, Subgraph sub, Graph_Func func) 
// {
   
// }
template<size_t y>
void myrun(const unsigned& x) {
    if constexpr(y==1) 
        fmt::print("working!\n");
}