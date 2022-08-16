//
// Created by supermt on 8/10/22.
//

#include "tables/cuckoo_table.h"

CuckooTable::CuckooTable(std::string fname, bool b) : Table(fname, b) {

}

int CuckooTable::FromDiskFormat(const Slice &data_pack, uint32_t *last_entry_count) {
 return 0;
}

int CuckooTable::ToDiskFormat(std::string *disk_format_buffer) {
 return 0;
}
