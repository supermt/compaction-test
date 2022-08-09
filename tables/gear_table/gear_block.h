//
// Created by supermt on 8/10/22.
//

#ifndef FPGA_OFFLOADING_LSM_GEAR_BLOCK_H
#define FPGA_OFFLOADING_LSM_GEAR_BLOCK_H

#include <string>
#include <memory>
#include <utility>
#include <slice.h>
#include <vector>

#define BLOCK_SIZE 8*1024
#define HEADER_LENGTH 64
#define KEY_CONTENT_LENGTH 8
#define KEY_SUFFIX_LENGTH 8


class gear_block {

public:
  explicit gear_block(int block_id) : num_of_entries(0), block_id_(block_id) {
  }

  int AppendToFile(int target_fd);

  int AddBlock(std::vector<Slice> keys, std::vector<Slice> values);

private:
  std::string content_block;
  std::string key_content;
  std::string value_content;
  int num_of_entries;
  const int block_id_;

  int ComposeResults();

};


#endif //FPGA_OFFLOADING_LSM_GEAR_BLOCK_H
