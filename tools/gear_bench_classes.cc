//
// Created by jinghuan on 7/12/21.
//
#include "gear_bench_classes.h"
#include "slice.h"

KeyGenerator::KeyGenerator(Random64 *rand, WriteMode mode, uint64_t num,
                           uint64_t seed, int key_size, uint64_t min_value)
    : rand_(rand), mode_(mode), distinct_num_(num), next_(min_value) {
 if (mode_ == UNIQUE_RANDOM) {
  values_.resize(distinct_num_);
  for (uint64_t i = min_value; i < distinct_num_; ++i) {
   values_[i] = i;
  }
  RandomShuffle(values_.begin(), values_.end(), static_cast<uint32_t>(seed));
 }
 key_size_ = key_size;
 key_slice_ = AllocateKey(&key_guard, key_size_);
}

// Static Functions
std::string KeyGenerator::GenerateKeyFromInt(uint64_t v, uint64_t num_keys,
                                             Slice *key, int key_size) {
 Slice key_results;
 std::unique_ptr<const char[]> temp_buf;
 AllocateKey(&temp_buf, key_size);
 if (key == nullptr) {
  key = &key_results;
 }
 v = v % num_keys;
 char *start = const_cast<char *>(key->data());
 char *pos = start;
 int bytes_to_fill = std::min(key_size - static_cast<int>(pos - start), 8);
 if (port::kLittleEndian) {
  for (int i = 0; i < bytes_to_fill; ++i) {
   pos[i] = (v >> ((bytes_to_fill - i - 1) << 3)) & 0xFF;
  }
 } else {
  memcpy(pos, static_cast<void *>(&v), bytes_to_fill);
 }
 pos += bytes_to_fill;
 if (key_size > pos - start) {
  memset(pos, '0', key_size - (pos - start));
 }
 return key->ToString();
}

Slice KeyGenerator::AllocateKey(std::unique_ptr<const char[]> *key_guard,
                                int key_size) {
 char *data = new char[key_size];
 const char *const_data = data;
 key_guard->reset(const_data);
 return Slice(key_guard->get(), key_size);
}

KeyGenerator::~KeyGenerator() = default;

uint64_t SeqKeyGenerator::Next() {
 next_++;
 return std::max(min_ + next_, std::numeric_limits<uint64_t>::max());
}