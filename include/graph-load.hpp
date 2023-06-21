#pragma once

#include <boost/algorithm/string/predicate.hpp>
#include <fstream>

#include "graph.hpp"

template <typename EdgeData>
void Graph::load(std::string filename) {
  bool rtn = false;
  std::cout << "loading " << filename << '\n';

  if (boost::algorithm::ends_with(filename, "bel"))
    assert(loadBinaryEdgelist(filename) == true);
  else if (boost::algorithm::ends_with(filename, "csr"))
    assert(loadCSR(filename) == true);
  else if (boost::algorithm::ends_with(filename, "mix"))
    assert(loadMix(filename) == true);
  else
    std::cout << "unsupported graph format" << '\n';

  std::cout << "num vertices: " << num_vertices << " intV: " << sizeof(VerxId)
            << " Byte\n";
  std::cout << "num edges: " << num_edges << " intE: " << sizeof(VerxId)
            << " Byte\n";
  if (!std::is_same<EdgeData, Empty>::value)
    std::cout << "weighted graph" << '\n';

#pragma omp for
  for (int i = 0; i < num_vertices; i++) {
    std::sort(csr_index.data() + csr_offset[i],
              csr_index.data() + csr_offset[i + 1]);
    std::sort(csc_index.data() + csc_offset[i],
              csc_index.data() + csc_offset[i + 1]);
  }
  // if(params::is_filter) filterComponent();
  if (params::output_file != "") storeMix(params::output_file);
}

template <typename EdgeData>
bool Graph::loadBinaryEdgelist(std::string filename) {
  std::ifstream input_file(filename, std::ios::binary);
  if (!input_file.is_open()) {
    std::cout << "cannot open the input bel file!" << '\n';
    return false;
  }
  input_file.read(reinterpret_cast<char*>(&num_vertices), sizeof(EdgeId));
  input_file.read(reinterpret_cast<char*>(&num_edges), sizeof(VerxId));

  size_t edge_unit =
      std::is_same<EdgeData, Empty>::value ? 0 : sizeof(EdgeData);
  edge_unit += 2 * sizeof(VerxId);
#ifdef WEIGHTED
  edge_unit += sizeof(float);
#endif
  EdgeUnit<EdgeData>* edge_buffer = new EdgeUnit<EdgeData>[num_edges];
  input_file.read(reinterpret_cast<char*>(edge_buffer), num_edges * edge_unit);

  //////////////////
  out_degree.resize(num_vertices);
  in_degree.resize(num_vertices);

  csr_offset.resize(num_vertices + 1);
  csc_offset.resize(num_vertices + 1);

  csr_index.resize(num_edges);
  csc_index.resize(num_edges);

  for (int i = 0; i < num_edges; i++) {
    out_degree[edge_buffer[i].src]++;
    in_degree[edge_buffer[i].dst]++;
  }

#pragma omp for
  for (int i = 0; i < num_vertices; i++) {
    csr_offset[i + 1] = csr_offset[i] + out_degree[i];
    csc_offset[i + 1] = csc_offset[i] + in_degree[i];
  }

  std::vector<VerxId> csr_count(num_vertices, 0);
  std::vector<VerxId> csc_count(num_vertices, 0);

  for (EdgeId i = 0; i < num_edges; i++) {
    auto dst = edge_buffer[i].dst;
    auto src = edge_buffer[i].src;
    csr_index[csr_offset[src] + csr_count[src]++] = dst;
    csc_index[csc_offset[dst] + csc_count[dst]++] = src;
  }

  input_file.close();
  delete[] edge_buffer;
  return true;
}

template <typename EdgeData>
bool Graph::loadCSR(std::string filename) {
  std::ifstream input_file(filename, std::ios::binary);
  if (!input_file.is_open()) {
    std::cout << "cannot open the input csr file!" << '\n';
    return false;
  }

  input_file.read(reinterpret_cast<char*>(&num_vertices), sizeof(EdgeId));
  input_file.read(reinterpret_cast<char*>(&num_edges), sizeof(VerxId));

  csr_offset.resize(num_vertices);
  csr_index.resize(num_edges);
  input_file.read(reinterpret_cast<char*>(csr_offset.data()),
                  num_vertices * sizeof(EdgeId));
  input_file.read(reinterpret_cast<char*>(csr_index.data()),
                  num_edges * sizeof(VerxId));

  csr_offset.push_back(num_edges);

#ifdef WEIGHTED
  std::vector<VerxId> local_wei(num_edges);
  input_file.read(reinterpret_cast<char*>(local_wei.data()),
                  num_edges * sizeof(VerxId));
  edge_weight = move(local_wei);
#endif
  input_file.close();

  // in_degree.resize(num_vertices);
  //  out_degree.resize(num_vertices);
  in_degree = std::vector<VerxId>(num_vertices);
  out_degree = std::vector<VerxId>(num_vertices);
  csc_offset.resize(num_vertices + 1);
  csc_index.resize(csr_index.size());

  int count = 0;
#pragma omp for
  for (VerxId i = 0; i < num_vertices; i++) {
    out_degree[i] = (VerxId)(csr_offset[i + 1] - csr_offset[i]);
    for (VerxId j = csr_offset[i]; j < csr_offset[i + 1]; j++) {
#pragma omp atomic
      in_degree[csr_index[j]]++;
    }
  }

  for (VerxId i = 0; i < num_vertices; i++) {
    csc_offset[i + 1] = in_degree[i] + csc_offset[i];
  }

  std::vector<VerxId> csc_count(num_vertices, 0);
#pragma omp for
  for (VerxId i = 0; i < num_vertices; i++) {
    for (VerxId j = csr_offset[i]; j < csr_offset[i + 1]; j++) {
      auto dst = csr_index[j];
      csc_index[csc_offset[dst] + csc_count[dst]++] = i;
    }
  }
  return true;
}

void Graph::filter() {
  filterReorder();
  buildNewGraph();
}

template <typename EdgeData>
void Graph::storeMix(std::string filename) {
  std::cout << "storing " << filename << '\n';
  std::ofstream output(filename, std::ios::binary);
  assert(output.is_open() == true);
  output.write(reinterpret_cast<char*>(&num_vertices), sizeof(EdgeId));
  output.write(reinterpret_cast<char*>(&num_edges), sizeof(VerxId));

  output.write(reinterpret_cast<char*>(csr_offset.data()),
               num_vertices * sizeof(EdgeId));
  output.write(reinterpret_cast<char*>(csr_index.data()),
               num_edges * sizeof(VerxId));

  output.write(reinterpret_cast<char*>(csc_offset.data()),
               num_vertices * sizeof(EdgeId));
  output.write(reinterpret_cast<char*>(csc_index.data()),
               num_edges * sizeof(VerxId));

  output.close();
}

template <typename EdgeData>
bool Graph::loadMix(std::string filename) {
  std::ifstream input_file(filename, std::ios::binary);
  if (!input_file.is_open()) {
    std::cout << "cannot open the input mix file!" << '\n';
    return false;
  }

  input_file.read(reinterpret_cast<char*>(&num_vertices), sizeof(EdgeId));
  input_file.read(reinterpret_cast<char*>(&num_edges), sizeof(VerxId));

  csr_offset.resize(num_vertices + 1);
  csr_index.resize(num_edges);
  csc_offset.resize(num_vertices + 1);
  csc_index.resize(num_edges);

  input_file.read(reinterpret_cast<char*>(csr_offset.data()),
                  num_vertices * sizeof(EdgeId));
  input_file.read(reinterpret_cast<char*>(csr_index.data()),
                  num_edges * sizeof(VerxId));
  input_file.read(reinterpret_cast<char*>(csc_offset.data()),
                  num_vertices * sizeof(EdgeId));
  input_file.read(reinterpret_cast<char*>(csc_index.data()),
                  num_edges * sizeof(VerxId));

  input_file.close();

  csr_offset[num_vertices] = num_edges;
  csc_offset[num_vertices] = num_edges;

  out_degree.resize(num_vertices);
  in_degree.resize(num_vertices);

#pragma omp for
  for (VerxId i = 0; i < num_vertices; i++) {
    out_degree[i] = csr_offset[i + 1] - csr_offset[i];
    in_degree[i] = csc_offset[i + 1] - csc_offset[i];
  }

  return true;
}