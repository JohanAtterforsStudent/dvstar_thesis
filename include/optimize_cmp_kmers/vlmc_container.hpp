#pragma once 

#include <functional>
#include <filesystem>
#include <limits.h>

#include "vlmc_from_kmers/kmer.hpp"

/*
  Stores VLMC (multiple k-mers) in a container. 
*/

using Kmer = vlmc::VLMCKmer; 

namespace container{
 
class VLMC_Container{

  public:
    VLMC_Container() = default;
    ~VLMC_Container() = default;
    VLMC_Container(const std::filesystem::path &path_to_bintree){}; 

    Kmer null_kmer{};
    virtual size_t size() const { return 0;};
    virtual void push(const Kmer &kmer){};
    virtual void for_each(const std::function<void(Kmer &kmer)> &){};
    virtual Kmer &get(const int i) { std::cout << "Hello from bad place" << std::endl; return null_kmer; };
    virtual Kmer &find(const Kmer &kmer) { std::cout << "Looking in a bad place" << std::endl; return null_kmer; }
    virtual Kmer &find(const std::string &kmer_data) { std::cout << "Searching in the wrong place" << std::endl; return null_kmer; }
    virtual int get_max_kmer_index() const { return INT_MAX; }
    virtual int get_min_kmer_index() const { return 0; }

};

class VLMC_vector : public VLMC_Container {

  private: 
    std::vector<Kmer> container{}; 

  public: 
    VLMC_vector() = default;
    ~VLMC_vector() = default; 

    VLMC_vector(const std::filesystem::path &path_to_bintree) {
      std::ifstream ifs(path_to_bintree, std::ios::binary);
      cereal::BinaryInputArchive archive(ifs);

      Kmer kmer{};

      while (ifs.peek() != EOF){
        archive(kmer);
        push(kmer);
      }
      ifs.close();
    } 

    size_t size() const override { return container.size(); }

    void push(const Kmer &kmer) override { container.push_back(kmer); }

    void for_each(const std::function<void(Kmer &kmer)> &f) override {
      for (auto kmer : container){
        f(kmer);
      }
    }

    Kmer &get(const int i) override { return container[i]; }

    int get_max_kmer_index() const override { return container.size() - 1; }
    int get_min_kmer_index() const override { return 0; }

    Kmer &find(const Kmer &kmer) override {
      for (size_t i = 0; i < container.size(); i++){
        if (container[i]==kmer) {
          return container[i]; 
        }
      }
      return null_kmer; 
    }

    Kmer &find(const std::string &kmer_data) override { 
      for (size_t i = 0; i < container.size(); i++){
        if (container[i].to_string() == kmer_data) {
          return container[i];
        }
      }
      return null_kmer; 
    }

};

class VLMC_multi_vector : public VLMC_Container {

  private: 
    std::vector<Kmer> container{}; 
    int c_size = 0;
    int max_kmer_index = 0;
    int min_kmer_index = INT_MAX;

  public: 
    VLMC_multi_vector() = default;
    ~VLMC_multi_vector() = default; 

    VLMC_multi_vector(const std::filesystem::path &path_to_bintree) {
      std::ifstream ifs(path_to_bintree, std::ios::binary);
      cereal::BinaryInputArchive archive(ifs);

      Kmer kmer{};

      while (ifs.peek() != EOF){
        archive(kmer);
        push(kmer);
      }
      ifs.close();
      
    } 

    size_t size() const override { return c_size; }

    void push(const Kmer &kmer) override { 
      int index = get_index_rep(kmer);
      if(index > max_kmer_index){
        container.resize(index + 10);
        max_kmer_index = index;
      } else if (index < min_kmer_index){
        min_kmer_index = index;
      }
      //Must be done after resize (resize invalidades all iterators)
      container[index] = kmer; 
      c_size = c_size + 1;
      }

    void for_each(const std::function<void(Kmer &kmer)> &f) override {
      for (auto kmer : container){
        f(kmer);
      }
    }

    Kmer &get(const int i) override { return container[i]; }

    int get_max_kmer_index() const override { return max_kmer_index; }
    int get_min_kmer_index() const override { return min_kmer_index; }

    Kmer &find(const Kmer &kmer) override {
      auto index = get_index_rep(kmer);
      if (container[index]==kmer){
        return container[index];
      }
      return null_kmer; 
    }

    Kmer &find(const std::string &kmer_data) override { 
      auto tmp_kmer = create_kmer(kmer_data);
      auto index = get_index_rep(tmp_kmer);
      if (container[index].to_string()==kmer_data){
        return container[index];
      }
      return null_kmer; 
    }

    int get_index_rep(const Kmer &kmer) {
      int integer_value = 0;
      int offset = 1;
      for (int i = kmer.length - 1; i >= 0; i--) {
        auto kmer_2_bits = kmer.extract2bits(i) + 1;
        integer_value += (kmer_2_bits * offset);
        offset *= 4;
      }
      return integer_value;
    }

    Kmer create_kmer(const std::string &kmer_string) {
      vlmc::VLMCTranslator kmer{static_cast<int>(kmer_string.size())};
      if (!kmer_string.empty()) {
        kmer.from_string(kmer_string);
      }
      return kmer.construct_vlmc_kmer();
    }   
};

}

