//
// Created by supermt on 8/10/22.
//

#include "tables/gear_table_builder.h"

GearTableBuilder::GearTableBuilder(std::string fname)
    : Table(std::move(fname)) {
 std::cout << "creating gear table" << std::endl;
}

int GearTableBuilder::ToDiskFormat(std::string *disk_format_buffer) {
 return 0;
}

int GearTableBuilder::FromDiskFormat(const Slice &data_pack, uint32_t *last_entry_count) {
 return 0;
}
