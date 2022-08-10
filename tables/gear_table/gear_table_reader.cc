//
// Created by supermt on 8/10/22.
//

#include "tables/gear_table_reader.h"
#include <unistd.h>
#include <fcntl.h>

int GearTableReader::ReadWholeFile(std::string *result, uint64_t read_length) {
 char *buffer = new char();
 int readed_bytes = read(fd, buffer, read_length);
 if (readed_bytes == -1) {
  assert(read_length != 0);
 }
 delete buffer;


 return 0;
}

GearTableReader::GearTableReader(const std::string &fname) : num_block(0), last_entry_count(0) {
 fd = open(fname.c_str(), O_RDONLY | O_DIRECT);
 assert(fd != -1);
}
