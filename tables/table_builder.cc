//
// Created by supermt on 8/10/22.
//

#include "tables/table.h"

Table::~Table() {
 fsync(target_fd);
 close(target_fd);
}

Table::Table(const std::string &fname, bool read) : file_name(fname) {
 if (read) { target_fd = open(fname.c_str(), O_RDONLY); }
 else { target_fd = open(fname.c_str(), O_CREAT | O_WRONLY, 0644); }

 assert(target_fd != -1);
}

int Table::WriteToDisk(const std::string &data_pack) const {
 auto write_bytes = write(target_fd, data_pack.data(), data_pack.size());
 assert(write_bytes != -1);
 Flush();
 return (int) write_bytes;
}

uint64_t
Table::ReadFromDisk(std::string &result_buffer, uint64_t file_length) const {
 char *buffer = new char[file_length];
 auto readed_bytes = read(target_fd, buffer, (size_t) file_length);
// *result_buffer = std::string(buffer);
 result_buffer = std::string(buffer, file_length);
 delete[] buffer;
 assert(readed_bytes != -1);
 return readed_bytes;
}

uint64_t Table::FromOnBoardBlocks(const Slice &data_pack, uint32_t *last) {
 uint64_t num_entries = 0;
 num_of_blocks = 0;
 const char *data_byte_array = data_pack.data();
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
   Slice key(data_pack.data() + BLOCK_SIZE -
             j * (KEY_CONTENT_LENGTH + KEY_SUFFIX_LENGTH),
             (KEY_SUFFIX_LENGTH + KEY_CONTENT_LENGTH));
   Slice value(data_pack.data() + j * VALUE_LENGTH, VALUE_LENGTH);
   key_list.push_back(key);
   value_list.push_back(value);
   num_entries++;
  }
 }
 return num_entries;
}

void Table::ToOnBoardFormat(std::string *result_buffer) const {
 auto *result_list = new std::vector<OnBoardBlock>();
 assert(result_buffer != nullptr);
 ComposeOnBoardBlock(key_list, value_list, result_list);
 for (const auto &result: *result_list) {
  result_buffer->append(result.content_block);
 }
 delete result_list;
}

void Table::Flush() const {
 fdatasync(target_fd);
 fsync(target_fd);
}


