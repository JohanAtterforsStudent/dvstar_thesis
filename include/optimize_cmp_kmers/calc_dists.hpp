#include <Eigen/Dense>
#include <chrono> 
#include <mutex>
#include <deque>

#include "cluster_container.hpp"
#include "vlmc_container.hpp"
#include "parallel.hpp"
#include "distances/dvstar.hpp"
#include "utils.hpp"

namespace calculate {

using matrix_t  = Eigen::MatrixXd;
using vlmc_c    = container::VLMC_Container;  
using kmer_pair = container::Kmer_Pair;

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

  auto rec_fun = [&](size_t left, size_t right) {
    distances(left, right) = fun(cluster_left.get(left), cluster_right.get(right)); 
  };

  utils::matrix_recursion(start_index, stop_index, 0, cluster_right.size(), rec_fun); 
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
      parallel::parallelize(cluster_left.size(), cluster_right.size(), fun, requested_cores);
      // parallel::sequential(cluster_left.size(), cluster_right.size(), fun, requested_cores);
      return distances; 
}

void calculate_kmer_buckets(size_t start_bucket, size_t stop_bucket, 
    matrix_t &dot_prod, matrix_t &left_norm, matrix_t &right_norm,
    container::Kmer_Cluster &cluster_left, container::Kmer_Cluster &cluster_right) {
  auto left_it = cluster_left.get_begin();
  std::advance(left_it, start_bucket);
  for (size_t i = start_bucket; i < stop_bucket; i++) {
    auto idx = left_it->first;
    auto right_it = cluster_right.find(idx);
    if (right_it != cluster_right.get_end()){
      distance::dvstar_kmer_major(left_it->second, right_it->second, dot_prod, left_norm, right_norm);
    }
    left_it++;
  }
}

void calculate_kmer_buckets_new(size_t start_bucket, size_t stop_bucket, 
    std::deque<distance::Intermediate_matrix_val> &out_q,
    container::Kmer_Cluster &cluster_left, container::Kmer_Cluster &cluster_right) {
  auto left_it = cluster_left.get_begin();
  std::advance(left_it, start_bucket);
  for (size_t i = start_bucket; i < stop_bucket; i++) {
    auto idx = left_it->first;
    auto right_it = cluster_right.find(idx);
    if (right_it != cluster_right.get_end()){
      distance::dvstar_kmer_major_new(left_it->second, right_it->second, out_q);
    }
    left_it++;
  }
}

void calculate_kmer_buckets_single(size_t start_bucket, size_t stop_bucket, 
    matrix_t &dot_prod, matrix_t &left_norm, matrix_t &right_norm,
    container::Kmer_Cluster &cluster) {
  auto left_it = cluster.get_begin();
  std::advance(left_it, start_bucket);
  for (size_t i = start_bucket; i < stop_bucket; i++) {
    distance::dvstar_kmer_major_single(left_it->second, left_it->second, dot_prod, left_norm, right_norm);
    left_it++;
  }
}

matrix_t calculate_distance_major(
    container::Kmer_Cluster &cluster_left, container::Kmer_Cluster &cluster_right,
    size_t requested_cores){

  std::mutex matricies_mutex;
  matrix_t distances = matrix_t::Zero(cluster_left.size(), cluster_right.size());
  matrix_t dot_prod = matrix_t::Zero(cluster_left.size(), cluster_right.size());
  matrix_t left_norm = matrix_t::Zero(cluster_left.size(), cluster_right.size());
  matrix_t right_norm = matrix_t::Zero(cluster_left.size(), cluster_right.size());

  auto fun = [&](size_t start_bucket, size_t stop_bucket) {
    std::deque<distance::Intermediate_matrix_val> results{};
    calculate_kmer_buckets_new(start_bucket, stop_bucket, results, cluster_left, cluster_right);
    std::lock_guard<std::mutex> guard(matricies_mutex);
    for(auto r : results){
      dot_prod(r.left_id, r.right_id) += r.dot_prod;
      left_norm(r.left_id, r.right_id) += r.left_norm;
      right_norm(r.left_id, r.right_id) += r.right_norm;
    }
  };
  
  parallel::parallelize(cluster_left.experimental_bucket_count(), fun, requested_cores);

  auto rec_fun = [&](size_t left, size_t right) {
    distances(left, right) = distance::normalise_dvstar(dot_prod(left, right), left_norm(left, right), right_norm(left, right));
  }; 

  auto norm_fun = [&](size_t start_vlmc, size_t stop_vlmc) {
    utils::matrix_recursion(start_vlmc, stop_vlmc, 0, cluster_right.size(), rec_fun); 
  };

  parallel::parallelize(cluster_left.size(), norm_fun, requested_cores); 

  return distances; 
}

matrix_t calculate_distance_major(
    container::Kmer_Cluster &cluster,
    size_t requested_cores){

  matrix_t distances = matrix_t::Zero(cluster.size(), cluster.size());
  matrix_t dot_prod = matrix_t::Zero(cluster.size(), cluster.size());
  matrix_t left_norm = matrix_t::Zero(cluster.size(), cluster.size());
  matrix_t right_norm = matrix_t::Zero(cluster.size(), cluster.size());

  auto fun = [&](size_t start_bucket, size_t stop_bucket) {
    calculate_kmer_buckets_single(start_bucket, stop_bucket, dot_prod, left_norm, right_norm, cluster);
  };
  parallel::parallelize(cluster.experimental_bucket_count(), fun, requested_cores);

  auto rec_fun = [&](size_t left, size_t right) {
    if(left > right) {
      distances(left, right) = 0.0;
      return;
    }
    distances(left, right) = distance::normalise_dvstar(dot_prod(left, right), left_norm(left, right), right_norm(left, right));
  }; 

  auto norm_fun = [&](size_t start_vlmc, size_t stop_vlmc) {
    utils::matrix_recursion(start_vlmc, stop_vlmc, 0, cluster.size(), rec_fun);
  };

  parallel::parallelize(cluster.size(), norm_fun, requested_cores); 

  return distances;
}

}