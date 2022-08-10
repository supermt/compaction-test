//
// Created by supermt on 8/10/22.
//

#include "tables/block_based_table.h"

BlockBasedTable::BlockBasedTable(std::string fname) : Table(fname) {

}

int BlockBasedTable::FromDiskFormat(const Slice &data_pack, uint32_t *last_entry_count) {
 return 0;
}

int BlockBasedTable::ToDiskFormat(std::string *disk_format_buffer) {
 return 0;
}
