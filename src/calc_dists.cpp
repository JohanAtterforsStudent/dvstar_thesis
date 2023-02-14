#include "parser.hpp"
#include "get_cluster.hpp"
#include "calc_dists.hpp"

using matrix_t = Eigen::MatrixXd;
using vlmc_c = container::VLMC_multi_vector;
using cluster_c = container::Cluster_Container;

int main(int argc, char *argv[]){
  CLI::App app{"Distance comparison of either one directory or between two different directories."};

  parser::cli_arguments arguments{};
  add_options(app, arguments);

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError &e) {
    return app.exit(e);
  }
  if(arguments.mode != parser::Mode::compare){
    return EXIT_SUCCESS;
  }
  if(arguments.first_VLMC_path.empty()){
    std::cerr
        << "Error: A input path to .bintree files has to be given for comparison operation."
        << std::endl;
    return EXIT_FAILURE;
  }
  auto distance_function = parser::parse_distance_function(arguments.dist_fn, arguments.background_order);
  size_t nr_cores_to_use = parser::parse_dop(arguments.dop);
  
  if(arguments.second_VLMC_path.empty()){
    if (arguments.cluster == parser::Cluster_Rep::cluster_vector){
      container::Cluster_vector cluster{};
      if (arguments.vlmc==parser::Vlmc_Rep::vlmc_vector){
        cluster::get_cluster<container::VLMC_vector>(arguments.first_VLMC_path, cluster); 
      } else if (arguments.vlmc==parser::Vlmc_Rep::vlmc_multi_vector){
        cluster::get_cluster<container::VLMC_multi_vector>(arguments.first_VLMC_path, cluster);
      } else {
        std::cerr
          << "Error: vlmc representation is not one that can be used :( "
          << std::endl;
        return EXIT_FAILURE;
      }
      matrix_t distance_matrix = calculate::calculate_distances(cluster, distance_function, nr_cores_to_use);
      
      for (size_t i = 0; i < cluster.size(); i++)
      {
        for (size_t j = 0; j < cluster.size(); j++)
        {
          std::cout << distance_matrix(i,j) << " ";
        }
        std::cout << std::endl;
      }
    } else {
      std::cerr
        << "Error: cluster representation is not one that can be used :( "
        << std::endl;
      return EXIT_FAILURE;
    }
  } else {
    if (arguments.cluster == parser::Cluster_Rep::cluster_vector){
      container::Cluster_vector left_cluster{};
      container::Cluster_vector right_cluster{};
      cluster::get_cluster<vlmc_c>(arguments.first_VLMC_path, left_cluster);
      cluster::get_cluster<vlmc_c>(arguments.second_VLMC_path, right_cluster);
      matrix_t distance_matrix = calculate::calculate_distances(left_cluster, right_cluster, distance_function, nr_cores_to_use);
    } else {
      std::cerr
        << "Error: cluster representation is not one that can be used :( "
        << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}