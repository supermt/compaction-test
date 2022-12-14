//
// Created by supermt on 8/10/22.
//

#ifndef FPGA_OFFLOADING_LSM_GEAR_TABLE_BUILDER_H
#define FPGA_OFFLOADING_LSM_GEAR_TABLE_BUILDER_H


#include "tables/table.h"

class GearTable : public Table {

public:

  int ToDiskFormat(std::string *disk_format_buffer) override;

  int FromDiskFormat(const Slice &data_pack, uint32_t *last_entry_count) override;

  explicit GearTable(const std::string &fname, bool read = false);

};


#endif //FPGA_OFFLOADING_LSM_GEAR_TABLE_BUILDER_H
