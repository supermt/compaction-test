//
// Created by jinghuan on 7/12/21.
//
#pragma once

#include <algorithm>
#include <array>
#include <cinttypes>
#include <iostream>
#include <map>
#include <string>
#include <tuple>
#include "random.h"
#include "slice.h"
#include <utility>
#include <set>

#include "tables/gear_table/gear_block.h"
#include "tables/gear_table/gear_table_builder.h"
#include "utils/kv_map.h"
#include "gear_bench_classes.h"
#include "db_format/dbformat.h"

enum WriteMode {
  RANDOM, SEQUENTIAL, UNIQUE_RANDOM
};

class KeyGenerator {
public:
  KeyGenerator(Random64 *rand, WriteMode mode, uint64_t num, uint64_t seed,
               int key_size, uint64_t min_value);

  virtual ~KeyGenerator();

  virtual uint64_t Next() {
   switch (mode_) {
    case SEQUENTIAL:
     return next_++;
    case RANDOM:
     return rand_->Next() % distinct_num_;
    case UNIQUE_RANDOM:
     assert(next_ < distinct_num_);
     return values_[next_++];
   }
   assert(false);
  }

  void SetNext(uint64_t next) { next_ = next; }

  std::string NextString() {
   return GenerateKeyFromInt(Next(), distinct_num_, &key_slice_, key_size_);
  }

  std::string GenerateKeyFromInt(uint64_t v) {
   return GenerateKeyFromInt(v, distinct_num_, &key_slice_, key_size_);
  }

  static std::string GenerateKeyFromInt(uint64_t v, uint64_t num_keys,
                                        Slice *key, int key_size = 15);

  static Slice AllocateKey(std::unique_ptr<const char[]> *key_guard,
                           int key_size = 15);

protected:
  Random64 *rand_;
  WriteMode mode_;
  uint64_t min_;
  const uint64_t distinct_num_;
  uint64_t next_;
  int key_size_ = 16;
  std::vector<uint64_t> values_;
  Slice key_slice_;
  std::unique_ptr<const char[]> key_guard;
};

class SeqKeyGenerator : public KeyGenerator {
public:
  explicit SeqKeyGenerator(uint64_t min_value)
      : KeyGenerator(nullptr, SEQUENTIAL, std::pow(256, 7), 0, 8, min_value) {};

  uint64_t Next() override;
};

class SpanningKeyGenerator {
  uint64_t lower_bound_;
  uint64_t upper_bound_;
  std::set<uint64_t> result_list;
  std::default_random_engine engine;

public:
  enum DISTRIBUTION_TYPE {
    kUniform, kOther
  };

public:
  SpanningKeyGenerator(uint64_t lower_bound, uint64_t upper_bound, int num_keys,
                       int seed = 0, DISTRIBUTION_TYPE distribution = kUniform)
      : lower_bound_(lower_bound), upper_bound_(upper_bound), engine(seed) {
   assert(lower_bound_ < upper_bound_);
   switch (distribution) {
    case kUniform: {
     std::uniform_int_distribution<uint64_t> distributor(lower_bound_,
                                                         upper_bound_);
     for (int i = 0; i < num_keys; i++) {
      result_list.insert(distributor(engine));
     }
     break;
    }
    case kOther:
     result_list.clear();
     break;
   }
  }

  stl_wrappers::KVMap GenerateContent(KeyGenerator *key_gen_ptr,
                                      SequenceNumber *seqno) {
   stl_wrappers::KVMap content;
   std::string value = "1234567890";
   for (auto key: result_list) {
    InternalKey ikey(key_gen_ptr->GenerateKeyFromInt(key), ++(*seqno),
                     kTypeValue);
    *seqno += 1;
    content.emplace(ikey.Encode().ToString(), value);
   }
   return content;
  }
};

