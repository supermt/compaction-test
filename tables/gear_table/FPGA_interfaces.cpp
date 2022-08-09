//
// Created by supermt on 8/10/22.
//

#include "FPGA_interfaces.h"

void FPGA_interfaces::CallBackOfFPGA(const Slice &result_pack, int last_entry_count) {

}

void FPGA_interfaces::SendToFPGA(int SST_num, FPGA_interfaces::Rule *function, std::vector<std::string> sst_content) {
 assert(sst_content.size() == SST_num);
}
