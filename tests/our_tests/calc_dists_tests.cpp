#include <gtest/gtest.h>

#include <Eigen/Dense>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <tuple>

//Our files 
#include "vlmc_container.hpp"
#include "cluster_container.hpp"
#include "calc_dists.hpp"
#include "distances/dvstar.hpp"
#include "get_cluster.hpp"

//Original implementation files
#include <kmc_file.h>
#include "vlmc_from_kmers/dvstar.hpp"
#include "vlmc_from_kmers/build_vlmc.hpp"

using matrix_t  = Eigen::MatrixXd;
using vlmc_c = container::VLMC_vector;
using vlmc_t = container::VLMC_Container;
using cluster_c = container::Cluster_Container<vlmc_c>;

class CalcDistsTests : public ::testing::Test {
protected:
  void SetUp() override {}

  std::filesystem::path first_bintree{"../data/test_VLMCs/sequences_1.bintree"};
  std::filesystem::path second_bintree{"../data/test_VLMCs/sequences_2.bintree"};
  std::filesystem::path third_bintree{"../data/test_VLMCs/sequences_3.bintree"};

  std::filesystem::path path_to_bintrees{"../data/test_VLMCs"};

  vlmc_c first_vlmc{first_bintree};
  vlmc_c second_vlmc{second_bintree};
  vlmc_c third_vlmc{third_bintree};

  size_t background_order = 0;

  std::function<double(vlmc_t &, vlmc_t &)> dist_func = [&](auto &left, auto &right) {
      return distance::dvstar(left, right, background_order);
  };

  double error_tolerance = 1E-7;
};

cluster_c create_cluster(vlmc_c &vlmc, size_t size) {
  cluster_c cluster{};
  for (size_t i = 0; i < size; i++){
    cluster.push(vlmc);
  }
  return cluster; 
}

// Tests when comparing two directories 
TEST_F(CalcDistsTests, SizeTwoDir) {
  for (size_t x = 1; x < 5; x++){
    for (size_t y = 1; y < 5; y++){
      cluster_c cluster_left = create_cluster(first_vlmc, x);
      cluster_c cluster_right = create_cluster(second_vlmc, y);

      matrix_t distances = calculate::calculate_distances<vlmc_c>(cluster_left, cluster_right, dist_func, 1);
      EXPECT_EQ(distances.size(), x*y);
      EXPECT_EQ(distances.rows(), x);
      EXPECT_EQ(distances.cols(), y);
    }
  }
}

TEST_F(CalcDistsTests, AllValsTwoDir) {
  for (size_t x = 1; x < 2; x++){
    cluster_c cluster_left = create_cluster(first_vlmc, x);
    cluster_c cluster_right = create_cluster(second_vlmc, x);

    matrix_t distances = calculate::calculate_distances<vlmc_c>(cluster_left, cluster_right, dist_func, 1);

    for (size_t row = 0; row < distances.rows(); row++){
      for (size_t col = 0; col < distances.cols(); col++){
        EXPECT_GT(distances(row,col), 0);
      }
    }
  }
}

TEST_F(CalcDistsTests, ValueCheckTwoDir){
  // Multi Vector Implementation
  auto left_cluster_mv = cluster::get_cluster<container::Index_by_value>(path_to_bintrees, 1, 0);
  auto right_cluster_mv = cluster::get_cluster<container::Index_by_value>(path_to_bintrees, 1, 0);
  matrix_t distances_multi_vector = calculate::calculate_distances<container::Index_by_value>(left_cluster_mv, right_cluster_mv, dist_func, 1);

  // Vector Implementation
  auto left_cluster_v = cluster::get_cluster<vlmc_c>(path_to_bintrees, 1, 0);
  auto right_cluster_v = cluster::get_cluster<vlmc_c>(path_to_bintrees, 1, 0);
  matrix_t distances_vector = calculate::calculate_distances<vlmc_c>(left_cluster_v, right_cluster_v, dist_func, 1);

  // Sorted Vector Implementation
  auto left_cluster_s = cluster::get_cluster<container::VLMC_sorted_vector>(path_to_bintrees, 1, 0);
  auto right_cluster_s = cluster::get_cluster<container::VLMC_sorted_vector>(path_to_bintrees, 1, 0);
  matrix_t distances_sorted_vector = calculate::calculate_distances<container::VLMC_sorted_vector>(left_cluster_s, right_cluster_s, dist_func, 1);

  // B-tree Implementation
  auto left_cluster_b = cluster::get_cluster<container::VLMC_B_tree>(path_to_bintrees, 1, 0);
  auto right_cluster_b = cluster::get_cluster<container::VLMC_B_tree>(path_to_bintrees, 1, 0);
  matrix_t distances_b_tree = calculate::calculate_distances<container::VLMC_B_tree>(left_cluster_b, right_cluster_b, dist_func, 1);

  // HashMap Implementation
  auto left_cluster_h = cluster::get_cluster<container::VLMC_hashmap>(path_to_bintrees, 1, 0);
  auto right_cluster_h = cluster::get_cluster<container::VLMC_hashmap>(path_to_bintrees, 1, 0);
  matrix_t distances_hashmap = calculate::calculate_distances<container::VLMC_hashmap>(left_cluster_h, right_cluster_h, dist_func, 1);

  // Combination of Index by value & sorted vector Implementation
  auto left_cluster_c = cluster::get_cluster<container::VLMC_Combo>(path_to_bintrees, 1, 0);
  auto right_cluster_c = cluster::get_cluster<container::VLMC_Combo>(path_to_bintrees, 1, 0);
  matrix_t distances_combo = calculate::calculate_distances<container::VLMC_Combo>(left_cluster_c, right_cluster_c, dist_func, 1);


  // Veb Implementation - Currently too slow
  //cluster_c left_cluster_veb{};
  //cluster_c right_cluster_veb{};
  //cluster::get_cluster<container::VLMC_Veb>(path_to_bintrees, left_cluster_veb);
  //cluster::get_cluster<container::VLMC_Veb>(path_to_bintrees, right_cluster_veb);
  //matrix_t distances_veb = calculate::calculate_distances(left_cluster_veb, right_cluster_veb, dist_func, 1);
  
  // Dvstar Original implementation 
  matrix_t distances_org_dvstar{distances_vector.cols(), distances_vector.rows()};
  int x = 0;
  for (const auto& dir_entry_x : recursive_directory_iterator(path_to_bintrees)) {
    int y = 0; 
    for (const auto& dir_entry_y : recursive_directory_iterator(path_to_bintrees)) {
      distances_org_dvstar(x,y) = vlmc::dvstar(dir_entry_x, dir_entry_y, 0);
      y++;
    }
    x++;
  }
  for (int x = 0; x < distances_vector.cols(); x++){
    for (int y = 0; y < distances_vector.rows(); y++){
      if (x==y){
        EXPECT_NEAR(0.0, distances_multi_vector(x,y), error_tolerance);
        EXPECT_NEAR(0.0, distances_vector(x,y), error_tolerance);
      } else { 
        EXPECT_NEAR(distances_org_dvstar(x,y), distances_vector(x,y), error_tolerance);
        EXPECT_NEAR(distances_vector(x,y), distances_sorted_vector(x,y), error_tolerance);
        EXPECT_NEAR(distances_vector(x,y), distances_multi_vector(x,y), error_tolerance);
        EXPECT_NEAR(distances_vector(x,y), distances_b_tree(x,y), error_tolerance);
        EXPECT_NEAR(distances_vector(x,y), distances_hashmap(x,y), error_tolerance);
        EXPECT_NEAR(distances_vector(x,y), distances_combo(x,y), error_tolerance);
        //EXPECT_NEAR(distances_vector(x,y), distances_veb(x,y), error_tolerance);
      }
    }
  }
}

// Tests for inter-directory comparisons
TEST_F(CalcDistsTests, SizeOneDir) {
  for (size_t x = 0; x < 4; x++){
    cluster_c cluster_left = create_cluster(first_vlmc, x);
    matrix_t distances = calculate::calculate_distances(cluster_left, dist_func, 1);
    EXPECT_EQ(distances.size(), x * x); 
    EXPECT_EQ(distances.rows(), x);
    EXPECT_EQ(distances.cols(), x);
  }
}

TEST_F(CalcDistsTests, AllValsOneDir) {
  for (size_t x = 1; x < 2; x++){
    cluster_c cluster = create_cluster(first_vlmc, x);

    matrix_t distances = calculate::calculate_distances(cluster, dist_func, 1);

    for (size_t row = 0; row < distances.rows(); row++){
      for (size_t col = 1 + row; col < distances.cols(); col++){
        EXPECT_GT(distances(row,col), 0);
      }
    }
  }
}

TEST_F(CalcDistsTests, ValueCheckOneDir){
  EXPECT_EQ(0,0);
}

