#include <gtest/gtest.h>

#include "../src/kmer.hpp"

#include <kmc_file.h>

#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>


class KmerTests : public ::testing::Test {
protected:
  void SetUp() override {}
  VLMCKmer create_kmer(std::string kmer_string) {
    VLMCTranslator kmer{static_cast<int>(kmer_string.size())};
    if (!kmer_string.empty()) {
      kmer.from_string(kmer_string);
    }

    return kmer.construct_vlmc_kmer();
  }

  VLMCKmer current_kmer{15, 0, {}};
  VLMCKmer prev_kmer{15, 0, {}};
};

TEST_F(KmerTests, VLMCKmerConstructor) {
  std::string kmer_string{"ACTGCTGATCGA"};
  auto created = create_kmer(kmer_string);

  EXPECT_EQ(created.to_string(), kmer_string);
}

TEST_F(KmerTests, VLMCKmerConstructorOdd) {
  std::string kmer_string{"ACTGCTGATCGAC"};
  auto created = create_kmer(kmer_string);
  EXPECT_EQ(created.to_string(), kmer_string);
}

TEST_F(KmerTests, VLMCKmerConstructor11) {
  std::string kmer_string{"ACTGCTGATCG"};
  auto created = create_kmer(kmer_string);
  EXPECT_EQ(created.to_string(), kmer_string);
}

TEST_F(KmerTests, VLMCKmerConstructor10) {
  std::string kmer_string{"ACTGCTGATC"};
  auto created = create_kmer(kmer_string);
  EXPECT_EQ(created.to_string(), kmer_string);
}

TEST_F(KmerTests, VLMCKmerConstructor9) {
  std::string kmer_string{"ACTGCTGAT"};
  auto created = create_kmer(kmer_string);
  EXPECT_EQ(created.to_string(), kmer_string);
}

TEST_F(KmerTests, IsPod) {
  EXPECT_FALSE(std::is_pod<CKmerAPI>::value);
  EXPECT_TRUE(std::is_pod<VLMCKmer>::value);
}

TEST_F(KmerTests, DifferingPosition) {
  auto current_kmer = create_kmer("AAAAAAATCATCAGT");
  auto prev_kmer = create_kmer("AAAAAAAGTCGTCAG");

  auto diff_pos =
      VLMCKmer::get_first_differing_position(current_kmer, prev_kmer);
  EXPECT_EQ(diff_pos, 7);
}

TEST_F(KmerTests, NoDifferingPosition) {
  auto current_kmer = create_kmer("AAAAAAATCATCAGT");
  auto prev_kmer = create_kmer("AAAAAAATCATCAGT");

  auto diff_pos =
      VLMCKmer::get_first_differing_position(current_kmer, prev_kmer);
  EXPECT_EQ(diff_pos, -1);
}

TEST_F(KmerTests, AllDifferingPosition) {
  auto current_kmer = create_kmer("AAAAAAATCATCAGT");
  auto prev_kmer = create_kmer("TTCTCTATCTCTCTC");

  auto diff_pos =
      VLMCKmer::get_first_differing_position(current_kmer, prev_kmer);
  EXPECT_EQ(diff_pos, 0);
}

TEST_F(KmerTests, LongDifferingPosition) {
  auto current_kmer =
      create_kmer("TACTAGCTACGATCATGCATGCATGCATGCAAAAAAATCATCAGT");
  auto prev_kmer = create_kmer("TACTAGCTACGATCATGCATGCATGCATGCAAAAAAAGTCGTCAG");

  auto diff_pos =
      VLMCKmer::get_first_differing_position(current_kmer, prev_kmer);
  EXPECT_EQ(diff_pos, 37);
}

void check_prefixes(VLMCKmer &current_kmer, std::string &kmer_string) {
  for (int i = kmer_string.size(); i >= 0; i--) {
    auto prefix = VLMCKmer::create_prefix_kmer(current_kmer, (uint32)i, 0, {});
    std::string expected_string = kmer_string.substr(0, i);

    EXPECT_EQ(expected_string, prefix.to_string()) << i;
  }
}

TEST_F(KmerTests, CreatePrefixKmer) {
  std::string kmer_string{"TACTAGCTACGATCAT"};
  auto current_kmer = create_kmer(kmer_string);

  check_prefixes(current_kmer, kmer_string);
}

TEST_F(KmerTests, CreatePrefixKmerOdd) {
  std::string kmer_string{"TCGTACGACTGACAA"};
  auto current_kmer = create_kmer(kmer_string);

  check_prefixes(current_kmer, kmer_string);
}

TEST_F(KmerTests, CreatePrefixKmerLong) {
  std::string kmer_string{"TACTAGCTACGATCATTACTAGCTACGATCATTACTAG"};
  auto current_kmer = create_kmer(kmer_string);

  check_prefixes(current_kmer, kmer_string);
}

TEST_F(KmerTests, SortTest) {
  auto from_kmer = create_kmer("ACTGCTGATCGATC");
  auto to_kmer = create_kmer("ACTGCTGATCGATA");

  EXPECT_LT(to_kmer, from_kmer);
}

TEST_F(KmerTests, SortTestZero) {
  auto from_kmer = create_kmer("ACTGCTGATCGATC");
  auto to_kmer = create_kmer("");

  EXPECT_LT(to_kmer, from_kmer);
}

TEST_F(KmerTests, SortTestDiffLength) {
  auto from_kmer = create_kmer("ACTGCTGATCGATC");
  auto to_kmer = create_kmer("ACTGCTGATCGA");

  EXPECT_LT(to_kmer, from_kmer);
}

TEST_F(KmerTests, Comparator) {
  auto from_kmer = create_kmer("ACTGCTGATCGATC");
  auto to_kmer = create_kmer("ACTGCTGATCGA");

  KMerComparator<14> comparator{};

  EXPECT_TRUE(comparator(to_kmer, from_kmer));
}

TEST_F(KmerTests, ReverseSortTest) {
  auto from_kmer = create_kmer("TTTTTTTTTT");
  auto to_kmer = create_kmer("TTTTTTTTTC");

  EXPECT_TRUE(from_kmer.reverse_less_than(to_kmer));
}

TEST_F(KmerTests, ReverseSortTestZero) {
  auto from_kmer = create_kmer("TTTTTTTTTT");
  auto to_kmer = create_kmer("");

  EXPECT_TRUE(from_kmer.reverse_less_than(to_kmer));
}

TEST_F(KmerTests, ReverseSortTestDiffLength) {
  auto from_kmer = create_kmer("TTTTTTTTTT");
  auto to_kmer = create_kmer("TTTTTTTTT");

  EXPECT_TRUE(from_kmer.reverse_less_than(to_kmer));
}

TEST_F(KmerTests, CountSort) {
  auto from_kmer = create_kmer("");
  auto to_kmer = create_kmer("");
  to_kmer.count = 50;

  EXPECT_TRUE(from_kmer.reverse_less_than(to_kmer));
}

TEST_F(KmerTests, ReverseSortComplex) {
  auto from_kmer = create_kmer("TAGCAAAAA");
  auto to_kmer = create_kmer("AAAAAAAAA");

  EXPECT_TRUE(from_kmer.reverse_less_than(to_kmer));
}

TEST_F(KmerTests, ReverseComparator) {
  auto from_kmer = create_kmer("TTTTTTTTTT");
  auto to_kmer = create_kmer("TTTTTTTTTC");
  ReverseKMerComparator<10> comparator{};

  EXPECT_TRUE(comparator(from_kmer, to_kmer));
}

TEST_F(KmerTests, ReverseSortTestSimilarEnds) {
  auto kmer_t = create_kmer("TTTTTTTTTT");
  auto kmer_a = create_kmer("TTTTTTTTTA");
  auto a_kmer = create_kmer("ATTTTTTTTT");

  std::vector<VLMCKmer> kmers{kmer_a, kmer_t, a_kmer};
  std::sort(kmers.begin(), kmers.end(), ReverseKMerComparator<10>());

  std::string first = kmers[0].to_string();
  std::string second = kmers[1].to_string();
  std::string third = kmers[2].to_string();

  EXPECT_EQ(first, "TTTTTTTTTT");
  EXPECT_EQ(second, "ATTTTTTTTT");
  EXPECT_EQ(third, "TTTTTTTTTA");
}

TEST_F(KmerTests, ReverseKey2) {
  auto ta_kmer = create_kmer("TA");
  auto gc_kmer = create_kmer("GC");
  auto at_kmer = create_kmer("AT");
  auto c_kmer = create_kmer("C");
  auto e_kmer = create_kmer("");
  KMerReverseKeyExtractor<2> comparator{};

  auto c_comparator = comparator(c_kmer);

  auto ta_comparator = comparator(ta_kmer);

  EXPECT_EQ(ta_comparator, 16) << ta_kmer.to_string();
  EXPECT_EQ(comparator(gc_kmer), 12);
  EXPECT_EQ(comparator(at_kmer), 4);


  EXPECT_EQ(comparator(c_kmer), 15);

  EXPECT_EQ(comparator(e_kmer), 21);
}


TEST_F(KmerTests, ReverseKey3) {
  auto ttt_kmer = create_kmer("TTT");
  auto tt_kmer = create_kmer("TT");
  auto aaa_kmer = create_kmer("AAA");
  auto a_kmer = create_kmer("A");
  auto e_kmer = create_kmer("");
  KMerReverseKeyExtractor<3> comparator{};

  EXPECT_EQ(comparator(ttt_kmer), 1);
  EXPECT_EQ(comparator(tt_kmer), 5);
  EXPECT_EQ(comparator(aaa_kmer), 82);

  EXPECT_EQ(comparator(a_kmer), 84);

  EXPECT_EQ(comparator(e_kmer), 85);
}

TEST_F(KmerTests, CerealSerialisable) {
  auto n_serializers = cereal::traits::detail::count_output_serializers<VLMCKmer, cereal::BinaryOutputArchive>::value;
  auto is_serializable = cereal::traits::is_output_serializable<std::vector<uint64>, cereal::BinaryOutputArchive>::value;
  EXPECT_TRUE(is_serializable);
  EXPECT_NE(n_serializers, 0);
}