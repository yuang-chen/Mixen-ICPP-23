/**
 *
 * This code implements work optimized propagation blocking with
 * transposed bin graph to reduce cache misses in scatter
 */
#include <pthread.h>
#include <time.h>

#include <boost/timer/timer.hpp>
#include <string>
//#include "global.hpp"
#include "graph.hpp"
#include "option.hpp"
#include "sort.hpp"

using namespace std;
using namespace boost::timer;
using namespace boost::program_options;

class PCPM_Func {
 public:
  Attr_t* pagerank = nullptr;  // [num_regular]
  Attr_t* pr_all = nullptr;    // [num_vertices]
  Attr_t* pr_cache = nullptr;  // [num_regular]
  VerxId* out_degree = nullptr;

  EdgeId size = 0;
  EdgeId num_reg = 0;
  Attr_t const damp = 0.85;

  PCPM_Func() {}
  ~PCPM_Func() {
    delete[] pr_all;
    delete[] pr_cache;
    delete[] pagerank;
  }

  ////////////////////////
  // functions for BIN mode
  ////////////////////////
  inline float scatterFunc(unsigned vertex)  // this one is used in both modes
  {
    return pagerank[vertex];
  }
  inline bool resetFunc(unsigned vertex) {
    pagerank[vertex] = pr_cache[vertex];
    return true;
  }
  inline bool gatherFunc(float update, unsigned vertex) {
    pagerank[vertex] += update;
    return true;
  }
  inline bool applyFunc(unsigned vertex) {
    pagerank[vertex] = 1 - damp + damp * pagerank[vertex];
    if (out_degree[vertex] > 0)
      pagerank[vertex] = pagerank[vertex] / out_degree[vertex];
    return true;
  }
  ///////////////////
  // helper functions
  //////////////////
  void init() {
#pragma omp for schedule(static, 1024)
    for (unsigned i = 0; i < num_reg; i++) {
      pagerank[i] = out_degree[i] > 0 ? (float)1 / out_degree[i] : 1.0;
    }
    //  std::fill(pr_all, pr_all + size, 0);
  }

  void init(Graph* g) {
    if (pr_all == nullptr) {  // not initialized yet
                              //   pr_all = new Attr_t [g->num_vertices]();
      num_reg = g->type_offset[0];
      auto const seed_offset = g->type_offset[1];
      size = g->num_vertices;
      pr_cache = new Attr_t[num_reg]();
      pagerank = new Attr_t[size]();

      out_degree = g->out_degree.data();

#pragma omp parallel for
      for (int i = 0; i < num_reg; i++) {
        pagerank[i] = (Attr_t)1 / g->out_degree[i];
        for (auto j = g->csc_offset[i]; j < g->csc_offset[i + 1]; j++) {
          auto const src = g->csc_index[j];
          if (num_reg < src && src < seed_offset)
            pr_cache[i] += (Attr_t)1 / g->out_degree[src];
        }
      }
    } else {
      //    std::fill(pr_all, pr_all + g->num_vertices, 0);
      std::fill(pagerank, pagerank + g->type_offset[0], 0);
    }
    std::cout << "Prephase is done" << '\n';
  }
  ///////////////////
  // verify the pagerank value
  //////////////////////
  void verify() {
#pragma omp parallel for schedule(static, 1024)
    for (unsigned i = 0; i < size; i++) {
      if (out_degree[i] > 0) pagerank[i] = pagerank[i] * out_degree[i];
    }
    std::sort(pagerank, pagerank + size, greater<Attr_t>());

    std::ofstream output("sorted_pr.txt");
    if (!output.is_open()) {
      std::cout << "cannot open txt file!" << std::endl;
      exit(1);
    }
    // only write 100 values
    for (unsigned i = 0; i < 100; i++) output << pagerank[i] << '\n';
    output.close();
    std::cout << "the sorted pagerank values are printed out!" << '\n';
  }
};

//////////////////////////////////////////
// main function
//////////////////////////////////////////
int main(int argc, char** argv) {
  // omp_set_nested(true);
  options(argc, argv);
  // graph object
  Graph graph;
  // Compute the preprocessing time
  cpu_timer timer;
  //////////////////////////////////////////
  // read csr file
  //////////////////////////////////////////
  graph.load(params::input_file);
  cout << timer.elapsed().wall / (1e9) << "s: reading done for "
       << params::input_file << '\n';
  timer.start();
  graph.storeMix(params::output_file);
      graph.filter();
      cout << timer.elapsed().wall/(1e9) << "s: filtering done for " <<
      params::input_file << '\n';

      graph.partition();
      cout << timer.elapsed().wall/(1e9) << "s: blocking done for " <<
      params::input_file << '\n';

    //  graph.setFrontier();

      //std::vector<float> pagerank(graph.num_vertices);
      PCPM_Func pr;

      float total_time = 0, current_time = 0;
      //////////////////////////////////////////
      // iterate for 20 epochs
      //////////////////////////////////////////
      for(int r = 0; r < params::rounds; r++) {
          int iter = 0;
          pr.init(&graph);
          graph.preploop(pr);
          timer.start();
          for(int i = 0; i < params::iters; i++)
              graph.run(pr);
          current_time = timer.elapsed().wall/(1e9);
          cout << params::input_file << ", processing time: " << current_time
          << '\n'; total_time += current_time; graph.postloop(pr);

      }
      cout << "average time: " << total_time / params::rounds <<
      "\n----------------------\n";

      pr.verify();

  return 0;
}
