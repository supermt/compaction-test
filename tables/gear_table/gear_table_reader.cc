//
// Created by supermt on 8/10/22.
//

#include "tables/gear_table_reader.h"
#include <unistd.h>
#include <fcntl.h>

int GearTableReader::ReadWholeFile(std::string *result, uint64_t file_length) {
 char *buffer = new char(file_length);
 int readed_bytes = read(fd, buffer, file_length);
 *result = std::string(buffer);
 delete buffer;
 assert(readed_bytes != -1);
 return readed_bytes;
}

GearTableReader::GearTableReader(const std::string &fname) : num_block(0), last_entry_count(0) {
 fd = open(fname.c_str(), O_RDONLY | O_DIRECT);
 assert(fd != -1);
}

GearTableReader::~GearTableReader() {
 close(fd);
}