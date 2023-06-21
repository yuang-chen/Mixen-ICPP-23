#pragma once
#include "global.hpp"

class Subgraph {
public:
    ///////////////
    //! basic subgraph info.
    ///////////////
    unsigned id;
    unsigned start;
    unsigned end;
    unsigned num_bins; // == num_subs
    unsigned num_inter;            // num of inter-edges

    float**    buffer=nullptr;
    unsigned** inters=nullptr;
    unsigned** dstver=nullptr;
    unsigned** spaver=nullptr;
    std::vector<unsigned> spaver_size;      // record the number of active dstver during the sparse phase
    std::vector<unsigned> spabuf_size;
    /////////////////
    //! local rows & cols for asyn. operations
    /////////////////
    unsigned num_vers;  // == end - start 
    unsigned num_edges;
    unsigned num_intra;  // num of intra-edges
    std::vector<unsigned> vers;   // [num_vers + 1] vertex set
    std::vector<unsigned> intra; // [num_intra] intra-edges
    ///////////////////////
    //! sub frontier
    ///////////////////////
    float    threshold;
    bool     is_sparse;
    unsigned num_act_vers;
    unsigned num_act_edge;               
    std::atomic<unsigned> num_act_bins;  
    std::vector<unsigned> act_vers;   // [num_vers]
    std::vector<unsigned> act_bins;    // [num_subs]
    std::vector<bool_byte> bin_ftr;    // [bun_subs]

    Subgraph():num_bins(0), 
                id(0),
                start(0),
                end(0),
                num_inter(0),
                num_vers(0),
                num_edges(0),
                num_intra(0),
                threshold(0.0),
                is_sparse(false),
                num_act_vers(0),
                num_act_edge(0),
                num_act_bins(0) {}
    
    ~Subgraph(){
        deleteArray2d(inters, num_bins);
        deleteArray2d(buffer, num_bins);
        deleteArray2d(dstver, num_bins);
        deleteArray2d(spaver, num_bins);
    }

    bool inline checkSparsity(){
        //((28.0 * (float)TD->activeEdges) > ((float)(TD->PNG->numEdges)*10.5 + 2.67*(float)TD->totalEdges + 4.0*(float)NUM_BINS)); 
        is_sparse = (28.0 * (float) num_act_edge < threshold);
        return is_sparse;
    }

};


