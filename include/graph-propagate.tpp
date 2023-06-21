#pragma once
/////////////////////////
//! scatter and gather
////////////////////////
#include <fstream>

template <class Graph_Func>
void Graph::run(Graph_Func& func) {
  computeDense(func);
}

template <class Graph_Func>
void Graph::computeDense(Graph_Func& func) {
  // cpu_timer timer;
  //   timer.start();
#pragma omp parallel for schedule(dynamic, 1) num_threads(params::threads)
  for (unsigned i = 0; i < num_subs; i++) {
    for (unsigned j = 0; j < num_subs; j++)
      scatterDense(func, subgraphs[i].buffer[j], subgraphs[i].inters[j],
                   inters_size[i][j]);
    for (auto v = subgraphs[i].start; v < subgraphs[i].end; v++)
      func.resetFunc(v);  // func.resetFuncDense(v);
  }

#pragma omp parallel for schedule(dynamic, 1) num_threads(params::threads)
  for (unsigned i = 0; i < num_subs; i++) {
    for (unsigned j = 0; j < num_subs; j++)
      gatherDense(func, subgraphs[j].buffer[i], subgraphs[j].dstver[i],
                  dstver_size[j][i]);
    for (int v = subgraphs[i].start; v < subgraphs[i].end; v++)
      func.applyFunc(v);
  }
}
/////////////////////
//!!!!!!!!!!!!!!!!!!!!!!!!! Dense mode !!!!!!!!!!!!!!!!!!!!!
///////////////////
template <class Graph_Func>
void Graph::gatherDense(Graph_Func& func, float* buf_bin, unsigned* dst_bin,
                        unsigned size) {
  unsigned index = MASK::MAX_UINT;
  for (unsigned i = 0; i < size; i++) {
    auto const curr_ver = dst_bin[i];
    index += (curr_ver >> MASK::MSB);
    func.gatherFunc(buf_bin[index], curr_ver & MASK::MAX_POS);
  }
}

template <class Graph_Func>
void Graph::scatterDense(Graph_Func& func, float* buf_bin, unsigned* inter_bin,
                         unsigned size) {
  unsigned index = 0;
  for (unsigned i = 0; i < size; i++)  // auto const dst_ver: inter_bin)
    buf_bin[index++] = func.scatterFunc(inter_bin[i]);
}

template <class Graph_Func>
void Graph::apply(Graph_Func& func, Subgraph& sub) {
  unsigned count = 0;

  for (unsigned i = 0; i < sub.num_act_vers; i++) {
    auto const vertex = sub.act_vers[i];
    ver_ftr[vertex] = func.applyFunc(vertex);
    if (ver_ftr[vertex]) {
      sub.num_act_edge += out_degree[vertex];
      sub.act_vers[count++] = vertex;
    }
  }
  sub.num_act_vers = count;
  if (count > 0) this->num_act_vers.fetch_add(count);
}

template <class Graph_Func>
void Graph::reset(Graph_Func& func, Subgraph& sub) {
  unsigned count = 0;
  for (unsigned i = 0; i < sub.num_act_vers; i++) {
    const auto vertex = sub.act_vers[i];
    const bool is_act = func.resetFunc(vertex);
    ver_ftr[vertex] = is_act;
    if (is_act) sub.act_vers[count++] = vertex;
  }
  if ((count > 0) &&
      __sync_bool_compare_and_swap(&sub_ftr[sub.id], false, true)) {
    sub_gather[num_act_subs.fetch_add(1)] = sub.id;
  }
  sub.num_act_vers = count;
}

template <class Graph_Func>
void Graph::preploop(Graph_Func& func) {
///////////////////
// cache the update of regular for following iterations
// initialize the regular componets
////////////////
#pragma omp parallel for
  for (EdgeId i = 0; i < type_offset[0]; i++) {
    func.pr_cache[i] = func.pr_cache[i] * 0.15;
  }
}

template <class Graph_Func>
void Graph::postloop(Graph_Func& func) {
#pragma omp for
  for (int i = type_offset[2]; i < type_offset[3]; i++) {
    func.pagerank[i] = 0;
    for (auto j = csc_offset[i]; j < csc_offset[i + 1]; j++) {
      // vertex id in sink_csc_index are different from their id in the original
      // graph hence they must be mapped back
      func.pagerank[i] += func.pagerank[csc_index[j]];
    }
    func.pagerank[i] = 0.15 + 0.85 * func.pagerank[i];
  }
}