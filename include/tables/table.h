//
// Created by supermt on 8/10/22.
//

#ifndef COMPACTION_TEST_TABLE_H
#define COMPACTION_TEST_TABLE_H

#include "db_format/dbformat.h"
#include <iostream>
#include <string>

#define ENTRIES_PER_FILE MAX_ENTRY_IN_BLOCK * 1000

class Table {
public:
  explicit Table(std::string fname);

  virtual ~Table();

  uint64_t ReadFromDisk(std::string *result_buffer, uint64_t read_length);

  uint64_t FromOnBoardBlocks(const Slice &data_pack, uint32_t *last_entry_count);

  void ToOnBoardFormat(std::string *result_buffer) const;

  virtual int ToDiskFormat(std::string *disk_format_buffer) = 0;

  virtual int FromDiskFormat(const Slice &data_pack, uint32_t *last_entry_count) = 0;

  int WriteToDisk(const Slice &data_pack);

  std::vector<Slice> key_list;
  std::vector<Slice> value_list;
  int target_fd;
  int num_of_blocks;
};


#endif //COMPACTION_TEST_TABLE_H