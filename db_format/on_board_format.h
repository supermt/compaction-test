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

  int AppendToFile(int target_fd);

  int AddBlock(const std::vector<Slice> &keys, const std::vector<Slice> &values);

private:
  std::string content_block;
  std::string key_content;
  std::string value_content;
  int num_of_entries;
  const int block_id_;

  int ComposeResults();

};
//
//struct EntryList {
//  std::vector<Slice> key_array;
//  std::vector<Slice> value_array;
//};

static void ComposeEntryVector(std::vector<Slice> *key_array,
                               std::vector<Slice> *value_array,
                               const Slice &input_block);

void ComposeOnBoardBlock(const std::vector<Slice> &keys, const std::vector<Slice> &values,
                         std::vector<OnBoardBlock> *block_list);

#endif //COMPACTION_TEST_GEAR_TABLE_FORMAT_H
