//
// Created by supermt on 8/10/22.
//

#include <unistd.h>
#include <fcntl.h>
#include "tables/gear_table_builder.h"
#include "utils/coding.h"

int GearTableBuilder::WriteToFile(const Slice &data_pack, uint32_t last_entry_count) {
 return write(target_fd, data_pack.data(), data_pack.size());
}

GearTableBuilder::GearTableBuilder(std::string fname) {
 num_of_blocks = 0;
 target_fd = open(fname.c_str(), (O_RDWR | O_CREAT));
 assert(target_fd != -1);
}

int GearTableBuilder::AddBlock(OnBoardBlock target_block) {
 target_block.AppendToFile(this->target_fd);
 num_of_blocks++;
 return num_of_blocks;
}

uint64_t GearTableBuilder::ParseFromDataPack(const Slice &data_pack, uint32_t *last) {
 uint64_t num_entries = 0;
 uint64_t current_key_length = 0;
 uint64_t current_value_length = 0;
 num_of_blocks = 0;
 const char *data_byte_array = data_pack.data();
 uint32_t page_entry_count;
 for (size_t i = 0; i < data_pack.size(); i += BLOCK_SIZE) {
  // iterate from the first byte
  Slice temp(data_byte_array + i, BLOCK_SIZE);
  // get the entry count from the file
  num_of_blocks++;
  uint32_t fixed_32_read_pos = 0;
  GetFixed32(&temp, &fixed_32_read_pos);
  GetFixed32(&temp, last);
  uint32_t key_length, value_length, placeholder_length = 0;
  GetFixed32(&temp, &key_length);
  GetFixed32(&temp, &value_length);
  GetFixed32(&temp, &placeholder_length);
  for (uint32_t j = 0; j < *last; j++) {
   Slice key(data_pack.data() + BLOCK_SIZE - j * (KEY_CONTENT_LENGTH + KEY_SUFFIX_LENGTH),
             (KEY_SUFFIX_LENGTH + KEY_CONTENT_LENGTH));
   Slice value(data_pack.data() + j * VALUE_LENGTH, VALUE_LENGTH);
   key_list.push_back(key);
   value_list.push_back(value);
   num_entries++;
  }
 }
 return num_entries;
}

GearTableBuilder::~GearTableBuilder() {
 close(target_fd);
}
