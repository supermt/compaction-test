//
// Created by supermt on 8/10/22.
//

#ifndef COMPACTION_TEST_FPGA_INTERFACES_H
#define COMPACTION_TEST_FPGA_INTERFACES_H


#include <string>
#include <vector>
#include "slice.h"

class FPGA_interfaces {
  typedef void **Rule;

  void SendToFPGA(int SST_num, Rule *function, std::vector<std::string> sst_content);

  void CallBackOfFPGA(const Slice &result_pack, int last_entry_count);
};


#endif //COMPACTION_TEST_FPGA_INTERFACES_H
