#include "graph.hpp"
#include <parallel/algorithm>
#include <parallel/numeric>


void Graph::filterReorder() {
    unsigned type = 4;
    auto max_threads = omp_get_max_threads();
    std::vector<unsigned>segment[max_threads][type];
    
    #pragma omp parallel for schedule(static) num_threads(max_threads)
    for(unsigned i = 0; i < num_vertices; i++) {
        switch ( (((out_degree[i]!=0) << 1) + (in_degree[i]!=0))  )
        {
        case 0: // isolated
            segment[omp_get_thread_num()][0].push_back(i);
            break;
        case 1: // seed
            segment[omp_get_thread_num()][1].push_back(i);
            break;        
        case 2: // sink
            segment[omp_get_thread_num()][2].push_back(i);
            break;      
         case 3: // regular
            segment[omp_get_thread_num()][3].push_back(i);
            break;

        default:
            std::cout << (((out_degree[i]!=0) << 1) + (in_degree[i]!=0))  << '\n';
            std::cout << "Wrong Graph Filtering!" << '\n';
            break;
        }
    }
    unsigned tmp = 0;
    unsigned seg_offset[max_threads][type];
    type_offset.resize(type);
    for(int j = type - 1; j >= 0; j--) {
        for(unsigned t = 0; t < max_threads; t++) {
            seg_offset[t][j] = tmp;
            tmp += segment[t][j].size();
        }
        type_offset[type-1 - j] = tmp;
    }


     new_id.resize(num_vertices);
        #pragma omp parallel for schedule(static) num_threads(max_threads)
        for(unsigned t = 0; t < max_threads; t++) {
            for(int j = type - 1; j >= 0; j--) {
                unsigned offset = seg_offset[t][j];
               // const auto& curr_seg = segment[t][j];
                for(auto id: segment[t][j])
                    new_id[id] = offset++;
            }
        }
}


void Graph::filterReorder2() {
    unsigned type = 5;
    auto max_threads = omp_get_max_threads();
    std::vector<unsigned>segment[max_threads][type];
    const auto aver = num_edges / num_vertices;
    
    #pragma omp parallel for schedule(static) num_threads(max_threads)
    for(unsigned i = 0; i < num_vertices; i++) {
        switch ( (((out_degree[i]!=0) << 1) + (in_degree[i]!=0))  )
        {
        case 0: // isolated
            segment[omp_get_thread_num()][0].push_back(i);
            break;
        case 1: // seed
            segment[omp_get_thread_num()][1].push_back(i);
            break;        
        case 2: // sink
            segment[omp_get_thread_num()][2].push_back(i);
            break;      
         case 3: // regular
            if(out_degree[i] > aver)    // hot
            {
                segment[omp_get_thread_num()][4].push_back(i);
            }
            else
            {
                segment[omp_get_thread_num()][3].push_back(i);
            }            break;

        default:
            std::cout << (((out_degree[i]!=0) << 1) + (in_degree[i]!=0))  << '\n';
            std::cout << "Wrong Graph Filtering!" << '\n';
            break;
        }
    }
    unsigned tmp = 0;
    unsigned seg_offset[max_threads][type];
    type_offset.resize(type);
    for(int j = type - 1; j >= 0; j--) {
        for(unsigned t = 0; t < max_threads; t++) {
            seg_offset[t][j] = tmp;
            tmp += segment[t][j].size();
        }
        type_offset[type-1 - j] = tmp;
    }


     new_id.resize(num_vertices);
        #pragma omp parallel for schedule(static) num_threads(max_threads)
        for(unsigned t = 0; t < max_threads; t++) {
            for(int j = type - 1; j >= 0; j--) {
                unsigned offset = seg_offset[t][j];
               // const auto& curr_seg = segment[t][j];
                for(auto id: segment[t][j])
                    new_id[id] = offset++;
            }
        }
}

void Graph::buildNewGraph() {
    #ifdef INFO
    boost::timer::cpu_timer timer;
    #endif
    std::cout << "--------build a new graph--------" << '\n';

    std::vector<VerxId> new_out_degree(num_vertices);
    std::vector<EdgeId> new_csr_offset(num_vertices+1);
    std::vector<VerxId> new_csr_index(num_edges);

    std::vector<VerxId> new_in_degree(num_vertices);
    std::vector<EdgeId> new_csc_offset(num_vertices+1);
    std::vector<VerxId> new_csc_index(num_edges);

 
    //Assign the outdegree to new id
    #pragma omp parallel for 
    for(unsigned i = 0; i < num_vertices; i++) {
        new_out_degree[new_id[i]] = out_degree[i];
        new_in_degree[new_id[i]] = in_degree[i];
    }
    // Build new row_index array
    __gnu_parallel::partial_sum(new_out_degree.begin(), new_out_degree.end(), new_csr_offset.begin() + 1);
    __gnu_parallel::partial_sum(new_in_degree.begin(), new_in_degree.end(), new_csc_offset.begin() + 1);


    std::cout << new_csr_offset.back() << " " << new_csc_offset.back()  << "\n";
    #ifdef WEIGHTED
        std::vector<unsigned> new_wei(num_edges, 0);
    #endif
    #ifdef INFO
    std::cout << "+" << timer.elapsed().wall/(1e9) << "s: new offsets are calculated" << '\n';
    #endif
    //Build new col_index array
    #pragma omp parallel for schedule(dynamic, 256) 
    for(unsigned i = 0; i < num_vertices; i++) {
        unsigned csr_count = 0;
        unsigned csc_count = 0;
        const auto roffset = new_csr_offset[new_id[i]];
        const auto coffset = new_csc_offset[new_id[i]];

        for(unsigned j = csr_offset[i]; j < csr_offset[i + 1]; j++) {
            new_csr_index[roffset + csr_count++] = new_id[csr_index[j]];
           // new_wei[new_csr_offset[new_id[i]] + csr_count] = edge_weight[j];
        }
        for(unsigned j = csc_offset[i]; j < csc_offset[i + 1]; j++) {
           new_csc_index[coffset + csc_count++] = new_id[csc_index[j]];
        }
    }   
    #ifdef INFO
    std::cout << "+" << timer.elapsed().wall/(1e9) << "s: new indexes are populated" << '\n';
    #endif
    std::swap(out_degree, new_out_degree);
    std::swap(csr_offset, new_csr_offset);
    std::swap(csr_index, new_csr_index);

    std::swap(in_degree, new_in_degree);
    std::swap(csc_offset, new_csc_offset);
    std::swap(csc_index, new_csc_index);


    #ifdef WEIGHTED
    new_graph.edge_weight.swap(new_wei);
    #endif
}