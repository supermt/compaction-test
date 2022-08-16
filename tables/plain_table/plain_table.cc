//
// Created by supermt on 8/10/22.
//

#include "tables/plain_table.h"

int PlainTable::ToDiskFormat(std::string *disk_format_buffer) {
 return 0;
}

int PlainTable::FromDiskFormat(const Slice &data_pack, uint32_t *last_entry_count) {
 return 0;
}

PlainTable::PlainTable(std::string fname, bool read) : Table(fname, read) {

}
