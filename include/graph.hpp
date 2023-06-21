#pragma once

#include "global.hpp"
#include "subgraph.hpp"

class Graph{
public:
    /////////////////////
    // basic graph components
    /////////////////////
    unsigned num_subs=0;
    unsigned num_vertices=0;
    unsigned num_edges=0;
    unsigned offset=0;
    
    std::vector<VerxId> new_id;
    std::vector<VerxId> type_offset;
 
    std::vector<EdgeId> csr_offset; // vertex index 
    std::vector<VerxId> csr_index;  // edge index
    std::vector<VerxId> out_degree;

    std::vector<EdgeId> csc_offset; // vertex index 
    std::vector<VerxId> csc_index;   // edge index
    std::vector<VerxId> in_degree;

    // components for subgraphs
    ////////////////////

     unsigned** dstver_size=nullptr;
     unsigned** inters_size=nullptr;   

    std::vector<unsigned> sub_map;    // [old num subs] map the new subgraphs to old subgraphs
    std::vector<unsigned> sub_offset; // [old num subs] offset for each subgraph after dynamic partitioning; used in locateSub 
    std::vector<Subgraph> subgraphs;  // [new num subs] new number = params::num_subgraphs, after dynamic partitioning
    /////////////////
    // components for frontier
    /////////////////
    bool already_cached = false;

  //  unsigned num_act_vers;
    std::atomic<unsigned> num_act_vers;
    std::atomic<unsigned> num_act_subs;    

    std::vector<unsigned> sub_scatter; //[num_subs] list of partitions with active vertices to scatter
    std::vector<unsigned> sub_gather; // [num_subs] list of partitions with active vertices to gather
    std::vector<unsigned> act_subs;     //  //
    std::vector<unsigned> act_vers;  // [num_vertices]
    
    std::vector<bool_byte> ver_ftr;     // [num_vertices] use uint_8 to mimic the bool
    std::vector<bool_byte> sub_ftr;      // [num_subs]  frontier for subgraphs
    std::vector<bool_byte> sub_scatter_done; // [num_subs]

#ifdef WEIGHTED
    std::vector<unsigned> edge_weight;
#endif
    Graph(unsigned num_vertices = 0, unsigned num_edges = 0):
            num_vertices(num_vertices),
            num_edges(num_edges),
            offset( (unsigned)log2(params::subgraph_size) ) {
            }

//    ~Graph(){
//       deleteArray2d(inters_size, num_subs);
//       deleteArray2d(dstver_size, num_subs);
//    }

    //////////////////
    // load
    // functions for load the dataset and constructing the basic graph data structure
    ///////////////
    template <typename EdgeData = Empty>
    void load(std::string filename); 
    template <typename EdgeData = Empty>
    bool loadMix(std::string filename); 
    template <typename EdgeData = Empty>
    bool loadBinaryEdgelist(std::string filename);
    template <typename EdgeData = Empty>
    bool loadCSR(std::string filename);
    template <typename EdgeData = Empty>
    void storeMix(std::string filename);
    void filterComponent();
    void filter();
    //////////////
    void filterReorder();
    void filterReorder2();
    void buildNewGraph();
    //////////////
    // partition.cpp
    // functions for partitioning and building the subgraphs
    //////////////
    void partition();
    void split();
    void dynamicSplit();
    void setSubgraph();
    void setFrontier(unsigned front_vertex = MASK::MAX_UINT);

    void calculateSkewLocality();
    void isOverflow();
    //////////////////////////
    // propagate.tpp
    // function for propagating the data
    // is it good to put every thing inside a class?
    //////////////////////////
    template<class Graph_Func> void run(Graph_Func& func);
    template<class Graph_Func> void preploop(Graph_Func& fun);
    template<class Graph_Func> void postloop(Graph_Func& fun);
    //! in dense mode: the most basic case
    template<class Graph_Func> void computeDense(Graph_Func& func);
    template<class Graph_Func> void scatterDense(Graph_Func& func, float* buf_bin, unsigned* inter_bin, unsigned size);
    template<class Graph_Func> void gatherDense(Graph_Func& func, float* buf_bin,  unsigned* dst_bin, unsigned size);
    //! in a mix of different modes
    template<class Graph_Func> void compute(Graph_Func& func);
    template<class Graph_Func> void scatter(Graph_Func& func, Subgraph& sub);
    template<class Graph_Func> void reset(Graph_Func& func, Subgraph& sub);
    template<class Graph_Func> void gather(Graph_Func& func, Subgraph& sub);
    template<class Graph_Func> void apply(Graph_Func& func, Subgraph& sub);
    //! in bin mode
    template<class Graph_Func> void scatterBin(Graph_Func& func, Subgraph& sub);
    template<class Graph_Func> void gatherBin(Graph_Func& func, Subgraph& sub, float* buf_bin, unsigned* dst_bin, unsigned size);
    //! in sparse mode
    template<class Graph_Func> void scatterSparse(Graph_Func& func, Subgraph& sub);
    template<class Graph_Func> void gatherSparse(Graph_Func& func, Subgraph& sub, float* buf_bin, unsigned* dst_bin, unsigned size);

    /////////
    // helper function
    ////////
    unsigned inline locateSub(unsigned vertex) {
        return vertex >> offset;
    }

    unsigned inline at(unsigned row, unsigned col) {
        return row * num_subs + col;
    }
};
///////////////////
// TODO: this thing is ugly
// maybe we should define a new class for the propagation
//////////////////
#include "graph-load.hpp"    
#include "graph-subdivide.hpp"    
#include "graph-propagate.tpp"    
#include "graph-reorder.hpp"    
