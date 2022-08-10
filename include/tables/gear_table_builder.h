//
// Created by supermt on 8/10/22.
//

#ifndef FPGA_OFFLOADING_LSM_GEAR_TABLE_BUILDER_H
#define FPGA_OFFLOADING_LSM_GEAR_TABLE_BUILDER_H


#include <unistd.h>
#include "db_format/dbformat.h"
#include "tables/table.h"
#include <vector>

class GearTableBuilder : public Table {

public:

  int ToDiskFormat(std::string *disk_format_buffer) override;

  int FromDiskFormat(const Slice &data_pack, uint32_t *last_entry_count) override;

  explicit GearTableBuilder(std::string fname);

  ~GearTableBuilder() override = default;
};


#endif //FPGA_OFFLOADING_LSM_GEAR_TABLE_BUILDER_H
