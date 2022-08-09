//
// Created by supermt on 8/10/22.
//

#ifndef FPGA_OFFLOADING_LSM_GEAR_TABLE_BUILDER_H
#define FPGA_OFFLOADING_LSM_GEAR_TABLE_BUILDER_H


#include <unistd.h>
#include "gear_block.h"
#include <vector>

class GearTableBuilder {
private:
  explicit GearTableBuilder(std::string fname);

  int target_fd;
  int num_of_blocks;

  std::vector<Slice> key_list;
  std::vector<Slice> value_list;

public:

  int AddBlock(gear_block target_block);
  // transform to other format, generate key/value map

  int WriteToFile(const Slice &data_pack, uint32_t last_entry_count);

  uint64_t ParseFromDataPack(const Slice &data_pack, uint32_t* last_entry_count);
};


#endif //FPGA_OFFLOADING_LSM_GEAR_TABLE_BUILDER_H
