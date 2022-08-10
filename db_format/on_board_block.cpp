//
// Created by supermt on 8/10/22.
//

#include "db_format/on_board_format.h"
#include <cassert>
#include <unistd.h>


int OnBoardBlock::AddBlock(const std::vector<Slice> &keys, const std::vector<Slice> &values) {
 assert(keys.size() == values.size());
 num_of_entries = keys.size();
 for (ssize_t i = 0; i < num_of_entries; i++) {
  key_content.append(keys[num_of_entries - i - 1].data(), keys[num_of_entries - i - 1].size());
  value_content.append(values[i].data(), values[i].size());
 }
 ComposeResults();
 return num_of_entries;
}

int OnBoardBlock::AppendToFile(int target_fd) {
 auto append_result = write(target_fd, content_block.c_str(), BLOCK_SIZE);
 if (append_result == -1) {
  assert(-1);
 }
 return (int) append_result;
}

int OnBoardBlock::ComposeResults() {
 uint32_t placeholder_length =
     BLOCK_SIZE - (HEADER_LENGTH +
                   key_content.size() + value_content.size());

 PutFixed32(&content_block, block_id_);
 PutFixed32(&content_block, num_of_entries);
 PutFixed32(&content_block, (uint32_t) key_content.size());
 PutFixed32(&content_block, (uint32_t) value_content.size());
 PutFixed32(&content_block, placeholder_length);

 // place holder
 PutFixed32(&content_block, 0);
 PutFixed32(&content_block, 0);
 PutFixed32(&content_block, 0);
 PutFixed32(&content_block, 0);
 PutFixed32(&content_block, 0);
 PutFixed32(&content_block, 0);
 PutFixed32(&content_block, 0);
 PutFixed32(&content_block, 0);
 PutFixed32(&content_block, 0);
 PutFixed32(&content_block, 0);
 PutFixed32(&content_block, 0);

 return 0;
}

void ComposeEntryVector(std::vector<Slice> *key_array, std::vector<Slice> *value_array, const Slice &input_blocks) {
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


void ComposeOnBoardBlock(const std::vector<Slice> &keys, const std::vector<Slice> &values,
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