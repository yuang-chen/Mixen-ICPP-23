#pragma once

#include "graph.hpp"
    // num_subgraphs X varied length 

void Graph::partition() {
    //boost::timer::cpu_timer timer;
    //timer.start();
    //////////////////////////////////////////
    // split the graph into subgraphs
    //////////////////////////////////////////
    //dynamicSplit();
    split();
  //  std::cout << timer.elapsed().wall/(1e9) << "s: split the graph into subgraphs "  << '\n';
    //////////////////////////////////////////
    // dstver_size =  vec2d<unsigned>(num_subs, std::vector<unsigned>(num_subs)); //initVec2d<unsigned>(num_subs, num_subs);
    // inters_size =  vec2d<unsigned>(num_subs, std::vector<unsigned>(num_subs));
    dstver_size = allocArray2d<unsigned>(num_subs, num_subs);
    inters_size = allocArray2d<unsigned>(num_subs, num_subs);

    setSubgraph();
   // std::cout << timer.elapsed().wall/(1e9) << "s: subgraphs are built "  << '\n';
 
}



void Graph::split() {
    VerxId subsize = params::subgraph_size * 1024 / sizeof(VerxId);
    auto num_verx = params::is_filter? type_offset[0]: num_vertices;
    num_subs = (num_verx - 1)/ subsize + 1; 
    offset = (unsigned) log2 (subsize); 
    subgraphs =  std::vector<Subgraph> (num_subs);

    /////////////////////////////
    // perform the dynamic partitioning based on the out-degrees 
    // when a subgraph is overflowed, i.e., sub_outdeg > 2 * average_deg,
    // such subgraph is further subdivided until the sub_outdegre <= 2 * average_deg
    /////////////////////////////
    #pragma omp parallel for schedule(static) num_threads(params::threads)
    for(unsigned n = 0; n < num_subs; n++) {
        auto& sub = subgraphs[n];
        sub.id = n;
        sub.start = n * subsize;
        sub.end = (n + 1) * subsize > num_verx? num_verx: (n + 1) * subsize;
    }
    // std::cout << "sub num: " << num_subs 
    //           << ", sub size:" << params::subgraph_size << " K vertices (" << subsize << " Byte)\n"; 
    
}
////////////////////////////////////////
// construct the subgraphs
// encode the destination vertices
////////////////////////////////////////
void Graph::setSubgraph() {
    boost::timer::cpu_timer timer;
 //   auto& row_offset = params::is_filter? regular_csr_offset: csr_offset;
  //  auto& col_index = params::is_filter? regular_csr_index: csr_index;
    auto& row_offset = csr_offset;
    auto& col_index =  csr_index;

    auto const row_border = type_offset[0];
    timer.start();
    #pragma omp parallel for schedule (dynamic, 1)
    for(unsigned n = 0; n < num_subs; n++) {
        Subgraph& sub = subgraphs[n];
        auto& dv_size = dstver_size[n];

        auto start = sub.start;
        auto end = sub.end;

        sub.num_vers = end - start;

        sub.vers = std::vector<unsigned>(sub.num_vers + 1);
        /////////////////////////////////////////
        // ! 1st traverse: 
        // get the dstver_size & sub.row & sub.loc_row for current subgraph sub[n]
        // dstver_size(n, m): how many (intra- & inter-) edges traveling from sub[n] to sub[m] -- uncompressed
        //////////////////////////////////////////
        for(unsigned i = start; i < end; i++) {
            unsigned prev_sub = num_subs + 1;
            for(unsigned j = row_offset[i]; j < row_offset[i+1]; j++) {
                if(col_index[j] >= row_border)
                    continue;
                unsigned curr_sub = locateSub(col_index[j]);
                dv_size[curr_sub]++;
                if(curr_sub == prev_sub)
                    continue;
                inters_size[n][curr_sub]++;
                prev_sub = curr_sub;
                sub.vers[i - start + 1] += (curr_sub == sub.id);
                sub.num_intra += (curr_sub == sub.id);
            }
        }
    }

  //  std::cout << timer.elapsed().wall/(1e9) << "s: 1st travesal "  << '\n';


    #pragma omp parallel for schedule(static ) num_threads(40)
    for(unsigned n = 0; n < num_subs; n++) {
        auto& sub = subgraphs[n];
        
        sub.buffer = new float* [num_subs];
        sub.dstver = new unsigned* [num_subs];
        sub.inters = new unsigned* [num_subs];
        sub.spaver = new unsigned* [num_subs];

        sub.spaver_size = std::vector<unsigned>(num_subs);
        sub.spabuf_size = std::vector<unsigned>(num_subs);

        for(unsigned m = 0; m <  num_subs; m++) {
            sub.inters[m] = new unsigned [inters_size[n][m]]();
            sub.buffer[m] = new float    [dstver_size[n][m]]();
            sub.dstver[m] = new unsigned [dstver_size[n][m]]();
            sub.spaver[m] = new unsigned [dstver_size[n][m]]();  
        }

    }
   //     std::cout << timer.elapsed().wall/(1e9) << "s: data allocation "  << '\n';

    // ! 2nd traverse:
    // * dstver: outgoing inter-edges
    // * encode inter-edges: curr_ver |= MASK::MAX_NEG;
    // A QUESTION: can we combine the 1st and 2nd traverse to reduce the preprocessing time?
        /////////////////////////////////////
    #pragma omp parallel for schedule(dynamic,1) num_threads(params::threads)
    for(int n = 0; n < num_subs; n++) { 
        auto& sub = subgraphs[n];
        unsigned dst_ver = 0;
        unsigned dst_sub = 0;
        unsigned prev_sub = 0;

        std::vector<unsigned> count_buffer(num_subs, 0);
        std::vector<unsigned> count_dstver(num_subs, 0);
        /////////////////////////////////////////
        // compute the subcol_index
        ///////////////////////////////////////

        sub.intra = std::vector<unsigned>(sub.num_intra);

        unsigned vertex_count = 0;
        std::vector<unsigned> count_sub(num_subs,0);

        for(auto i = sub.start; i < sub.end; i++) {
            prev_sub = num_subs;
            for(auto j = row_offset[i]; j < row_offset[i+1]; j++) {
                dst_ver = col_index[j];
                if(dst_ver >= row_border)
                    continue;
                dst_sub = locateSub(col_index[j]);
                if(prev_sub != dst_sub) {                           // subgraph[n].inters[s][x] = i
                    sub.inters[dst_sub][count_sub[dst_sub]++] = i;  // record the src vertex i in subgraph n who has outgoing inter-edge to subgraph s in sequence
                    dst_ver |= MASK::MAX_NEG; 
                    prev_sub = dst_sub;
                    if(dst_sub == sub.id) {
                        sub.intra[vertex_count++] = col_index[j];
                    } 
                }
                sub.dstver[dst_sub][count_dstver[dst_sub]++] = dst_ver; // the destination vertex are encoded and recorded here
            }
        }
    }
  //  std::cout << timer.elapsed().wall/(1e9) << "s: 2nd travesal "  << '\n';
}

void Graph::setFrontier(unsigned root) {
    /////////////////
    // in dense mode, we do not need frontier
    ////////////////
    
    sub_scatter = std::vector<unsigned>(num_subs, 0);
    sub_gather = std::vector<unsigned>(num_subs, 0);
    ver_ftr = std::vector<bool_byte>(num_vertices, false);
    sub_ftr = std::vector<bool_byte>(num_subs, false);
    sub_scatter_done = std::vector<bool_byte>(num_subs, false);
    num_act_subs = 0;

   // all_bin_ftr = getVec2d<bool_byte>(num_subs, num_subs);
  //  all_act_bins = getVec2d<unsigned>(num_subs, num_subs);

    act_vers = std::vector<unsigned>(num_vertices);
    act_subs = std::vector<unsigned>(num_subs);

///////////////////////////////////////////
// there are three cases for intializing the frontier
// 1. one vertex  -- the root vertex is active, then the frontier expands and finally converge to 0
// 2. all vertices --
//  2.1: all vertices are active in the beginning, and then converge to 0
//  2.2: all vertices *are active during the whole computation* -- we actually do not need frontier in such case
///////////////////////////////////////////
    #pragma omp parallel for
    for(unsigned i = 0; i < num_subs; i++) {
        auto& sub = subgraphs[i];
        sub.act_vers = std::vector<unsigned>(sub.end - sub.start, 0);
        sub.num_act_vers = 0;
        sub.num_act_edge = 0;
        sub.num_act_bins = 0;
        sub.is_sparse = false;
        sub.num_bins = num_subs;
        sub.num_inter = 0;
        sub.num_edges = 0;

        sub.bin_ftr  = std::vector<bool_byte>(num_subs);
        sub.act_bins = std::vector<unsigned>(num_subs); 



        for(unsigned j = 0; j < num_subs; j++) {
            // sub.num_inter += sub.inters[j].size();
            // sub.num_edges += sub.dstver[j].size();
            sub.num_inter += inters_size[i][j];
            sub.num_edges += dstver_size[i][j];
        }
        sub.threshold = 10.5*(float)sub.num_inter + 2.67*(float)sub.num_edges + 4.0*(float)sub.num_bins;
    }
    // activate root vertices
    if(root != MASK::MAX_UINT) {
        num_act_subs = 1;
        num_act_vers = 1;
       // front = std::vector<bool_byte>(this->num_vertices, false);
        ver_ftr[root] = true;
        auto const n = locateSub(root);
        
        subgraphs[n].act_vers[0] = root;
        subgraphs[n].num_act_vers = 1;
        subgraphs[n].num_act_edge = this->out_degree[root];
        sub_scatter[0] = n;
        //sub_ftr[n] = true;
    } else {
        ///////////////////////////////////
        // set all vertices to be active
        // take them all into frontier
        ///////////////////////////////////
        num_act_subs = num_subs;
        num_act_vers = this->num_vertices;
        ver_ftr = std::vector<bool_byte>(this->num_vertices, true);

        //#pragma omp for 
        for(unsigned n = 0; n < num_subs; n++) {
            Subgraph& sub = subgraphs[n];

            sub.num_act_vers = sub.end - sub.start;
            sub.num_act_edge = sub.num_inter;
            //sub.num_act_bins = num_subs; we set bin during the progatation
            sub_scatter[n] = n;
            sub_gather[n] = n;
           // sub_ftr[n] = true;

            #pragma omp simd
            for(unsigned i = 0; i < sub.num_vers; i++) {
                sub.act_vers[i] = sub.start + i;
            }
        }
    }
} 


