//
// Created by supermt on 8/11/22.
//

#include <iomanip>
#include <sstream>
#include <deque>
#include <queue>
#include "merger.h"
#include "tables/block_based_table.h"
#include "tables/cuckoo_table.h"
#include "tables/gear_table_builder.h"
#include "tables/plain_table.h"


// the stream merger is the merger that will transport piece by piece, and wait for the results from FPGA
FPGA_Stream_Merger::FPGA_Stream_Merger(std::vector<std::string> input_files, TableFormat format,
                                       FileNameCreator *fileNameHandler,
                                       ssize_t memory_budget)
    : Merger(input_files, format, fileNameHandler) {
 uint block_num = memory_budget / BLOCK_SIZE;
 file_window_size = BLOCK_SIZE * block_num;
}

inline void TransportToFPGA(const char *buffer, uint64_t data_size) {
 // the function that send data buffer to the FPGA
 if (data_size == 0) {
  return;
 }
}

inline void WaitForFPGA() {}

inline void InitialArbitrationCondition(FilterLogic arbitration, FilterArgs values) {
 // pass the arguments for the arbitration
// enum FilterLogic : int {
//   kRemoveRedundant = 0x0,
//   kDeletePrefix,
//   kDeleteVersion,
// };


// struct FilterArgs {
//   SequenceNumber seq;
//   Slice prefix;
// };
}

uint64_t FPGA_Stream_Merger::DoCompaction() {
 //read the files with a fixed file_window_size
 int read_bytes = -1;


 while (read_bytes != 0) {
  read_bytes = 0;
  buffer.clear();
  for (ssize_t fid = 0; fid < input_files.size(); fid++) {
   std::string read_buffer;
   int current_ouput = input_files[fid]->ReadFromDisk(&read_buffer, file_window_size);
   std::reverse(read_buffer.begin(), read_buffer.end());
   std::reverse(read_buffer.begin(), read_buffer.end());

   buffer.append(read_buffer);
   read_bytes = std::max(read_bytes, current_ouput);
  }
  read_bytes = buffer.size();
  TransportToFPGA(buffer.data(), read_bytes); // make this async
 }
 WaitForFPGA();

 // write back the results
 WriteOutResult();
 return 0;
}
