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

int OnBoardBlock::AppendToFile(int target_fd) const {
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
 content_block.append(value_content);
 std::string placeholder(placeholder_length, 'h');
 content_block.append(key_content);

 return 0;
}
