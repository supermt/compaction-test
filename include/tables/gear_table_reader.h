//
// Created by supermt on 8/10/22.
//

#ifndef COMPACTION_TEST_GEAR_TABLE_READER_H
#define COMPACTION_TEST_GEAR_TABLE_READER_H


#include <string>
#include "db_format/dbformat.h"

class GearTableReader {

public:
  int ReadWholeFile(std::string *result, uint64_t read_length);

  explicit GearTableReader(const std::string &fname);

private:
  int fd;
  int num_block;
  int last_entry_count;

};


#endif //COMPACTION_TEST_GEAR_TABLE_READER_H
