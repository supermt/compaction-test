//
// Created by supermt on 8/10/22.
//

#ifndef COMPACTION_TEST_GEAR_TABLE_FORMAT_H
#define COMPACTION_TEST_GEAR_TABLE_FORMAT_H

#include <vector>
#include "slice.h"
#include "utils/coding.h"

#define BLOCK_SIZE 8*1024
#define HEADER_LENGTH 64
#define KEY_CONTENT_LENGTH 8
#define KEY_SUFFIX_LENGTH 8
#define VALUE_LENGTH 10
#define FULL_KEY_LENGTH KEY_CONTENT_LENGTH + KEY_SUFFIX_LENGTH
#define MAX_ENTRY_IN_BLOCK 312

class OnBoardBlock {
public:
  explicit OnBoardBlock(int block_id) : num_of_entries(0), block_id_(block_id) {
  }

  int AppendToFile(int target_fd) const;

  int AddBlock(const std::vector<Slice> &keys, const std::vector<Slice> &values);

  int ComposeResults();

  std::string content_block;
  std::string key_content;
  std::string value_content;
  int num_of_entries;
  const int block_id_;
};

static void
ComposeEntryVector(std::vector<Slice> *key_array, std::vector<Slice> *value_array, const Slice &input_blocks) {
 uint64_t data_length = input_blocks.size();
 uint32_t total_entries;
 for (uint64_t i = 0; i < data_length; i += BLOCK_SIZE) {
  Slice current_block = Slice(input_blocks.data_ + i, BLOCK_SIZE);
  // read the header first
  uint32_t block_id;
  uint32_t block_entries;
  uint32_t key_content_size;
  uint32_t value_content_size;
  uint32_t placeholder_length;
  uint32_t useless_attributes;
  GetFixed32(&current_block, &block_id);
  GetFixed32(&current_block, &block_entries);
  GetFixed32(&current_block, &key_content_size);
  GetFixed32(&current_block, &value_content_size);
  GetFixed32(&current_block, &placeholder_length);

  // following attributes are useless
  GetFixed32(&current_block, &useless_attributes);
  GetFixed32(&current_block, &useless_attributes);
  GetFixed32(&current_block, &useless_attributes);

  GetFixed32(&current_block, &placeholder_length);
  GetFixed32(&current_block, &useless_attributes);
  GetFixed32(&current_block, &useless_attributes);
  GetFixed32(&current_block, &useless_attributes);
  total_entries += block_entries;
  // read the keys back forward
  for (uint32_t entry_posi = 0; entry_posi < block_entries; entry_posi++) {
   key_array->push_back(Slice(current_block.data() + BLOCK_SIZE - entry_posi * FULL_KEY_LENGTH, FULL_KEY_LENGTH));
   value_array->push_back(Slice(current_block.data() + entry_posi * VALUE_LENGTH, VALUE_LENGTH));
  }
 }
 assert(key_array->size() == total_entries);
}


static void ComposeOnBoardBlock(const std::vector<Slice> &keys, const std::vector<Slice> &values,
                                std::vector<OnBoardBlock> *block_list) {
 int block_id = 0;
 for (size_t i = 0; i < keys.size(); i += MAX_ENTRY_IN_BLOCK) {
  assert(keys.size() == values.size());
  size_t end_posi = i + MAX_ENTRY_IN_BLOCK > keys.size() ? keys.size() : i + MAX_ENTRY_IN_BLOCK;
  auto key_first = keys.begin() + i;
  auto key_end = keys.begin() + end_posi;
  std::vector<Slice> sub_key(key_first, key_end);

  auto value_first = values.begin() + i;
  auto value_end = values.begin() + end_posi;

  std::vector<Slice> sub_value(value_first, value_end);
  block_list->emplace_back(block_id);
  block_list->back().AddBlock(sub_key, sub_value);
 }

}

#endif //COMPACTION_TEST_GEAR_TABLE_FORMAT_H
