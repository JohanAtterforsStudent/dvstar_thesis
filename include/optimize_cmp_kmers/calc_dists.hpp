#include <Eigen/Dense>
#include "cluster_container.hpp"
#include "vlmc_container.hpp"
#include "parallel.hpp"

namespace calculate {

using matrix_t  = Eigen::MatrixXd;
using vlmc_c    = container::VLMC_Container;  

template <typename VC>
void calculate_reduced_slice(size_t start_index, size_t stop_index, matrix_t &distances,
                     container::Cluster_Container<VC> &cluster_left, container::Cluster_Container<VC> &cluster_right,
                     const std::function<double(vlmc_c &, vlmc_c &)> &fun) {
  size_t y_bound = 1;
  for (size_t i = start_index; i < stop_index; i++) {
    for (size_t j = y_bound; j < cluster_right.size(); j++) {
      distances(i, j) = fun(cluster_left.get(i), cluster_right.get(j));
    }
    y_bound++;
  }
}

template <typename VC>
void calculate_full_slice(size_t start_index, size_t stop_index, matrix_t &distances,
                     container::Cluster_Container<VC> &cluster_left, container::Cluster_Container<VC> &cluster_right,
                     const std::function<double(vlmc_c &, vlmc_c &)> &fun) {
  for (size_t i = start_index; i < stop_index; i++) {
    for (size_t j = 0; j < cluster_right.size(); j++) {
      distances(i, j) = fun(cluster_left.get(i), cluster_right.get(j));
    }
  }
}

// Inter-directory distances
template <typename VC> 
matrix_t calculate_distances(
    container::Cluster_Container<VC> &cluster, std::function<double(vlmc_c &, vlmc_c &)> &distance_function,
    size_t requested_cores){

  matrix_t distances{cluster.size(), cluster.size()};

  auto fun = [&](size_t start_index, size_t stop_index) {
    calculate_reduced_slice<VC>(start_index, stop_index, distances,
                           cluster, cluster, distance_function);
  };
  //TODO use parallelize
  parallel::parallelize(cluster.size(), fun, requested_cores);
  return distances; 
}

// For two different dirs
template <typename VC>
matrix_t calculate_distances(
    container::Cluster_Container<VC> &cluster_left, container::Cluster_Container<VC> &cluster_right,
    std::function<double(vlmc_c &, vlmc_c &)> &distance_function,
    size_t requested_cores){

      matrix_t distances{cluster_left.size(), cluster_right.size()};

      auto fun = [&](size_t start_index, size_t stop_index) {
      calculate_full_slice<VC>(start_index, stop_index, std::ref(distances),
                           std::ref(cluster_left), std::ref(cluster_right), distance_function);
      };
      //TODO use parallelize
      parallel::parallelize(cluster_left.size(), cluster_right.size(), fun, requested_cores);
      // parallel::sequential(cluster_left.size(), cluster_right.size(), fun, requested_cores);
      return distances; 
}
}