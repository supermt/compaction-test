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

BaselineMerger::BaselineMerger(std::vector<std::string> input_files, FileNameCreator *fileNameCreator) : Merger(
    input_files, kPlain, fileNameCreator) {
}

inline int NextEntry(PlainTable *table, std::string *entry) {
 int readed_bytes = table->ReadFromDisk(entry, FULL_KEY_LENGTH + VALUE_LENGTH);
 return readed_bytes / (FULL_KEY_LENGTH + VALUE_LENGTH);
}


namespace stream_merger_heap {
  typedef std::pair<std::string, PlainTable *> entry_file_pair;

  class KeyComparor {
  public:
    int operator()(const entry_file_pair &p1, const entry_file_pair &p2) {

     ParsedInternalKey *p1_key = new ParsedInternalKey();
     ParsedInternalKey *p2_key = new ParsedInternalKey();
     ParseInternalKey(Slice(p1.first.data(), FULL_KEY_LENGTH), p1_key);
     ParseInternalKey(Slice(p1.first.data(), FULL_KEY_LENGTH), p2_key);

     int r = p1_key->user_key.compare(p2_key->user_key);
     if (r == 0) {
      if (p1_key->sequence > p2_key->sequence) {
       r = -1;
      } else if (p1_key->sequence < p2_key->sequence) {
       r = +1;
      } else if (p1_key->type > p2_key->type) {
       r = -1;
      } else if (p1_key->type < p2_key->type) {
       r = +1;
      }
     }

     return r;
    }
  };

  typedef std::priority_queue<entry_file_pair, std::vector<entry_file_pair>, KeyComparor> entry_file_heap;

};

BaselineMerger::ArbitrationAction BaselineMerger::Arbitration(const std::string &current_entry) {
 // return -1 for last entry has been removed
 // return 0 for nothing happen
 // return 1 for current entry will be removed
 ParsedInternalKey last;
 ParsedInternalKey current;
 ParseInternalKey(Slice(last_entry.data(), FULL_KEY_LENGTH), &last);
 ParseInternalKey(Slice(current_entry.data(), FULL_KEY_LENGTH), &current);

 auto temp = last_entry;
 last_entry = current_entry;
 switch (logic_) {
  case kRemoveRedundant: {
   if (last.user_key == current.user_key) {
    abandoned_entries.push_back(temp);
    return kDeleteLast;
   } else {
    return kAcceptEntry;
   }
  }
  case kDeletePrefix: {
   if (current.user_key.starts_with(bound_.prefix)) {
    abandoned_entries.push_back(last_entry);
    return kDeleteCurrent;
   } else {
    return kAcceptEntry;
   }
  }
  case kDeleteVersion: {
   if (last.user_key.starts_with(bound_.prefix) && last.user_key == current.user_key) {
    // redundant keys, within the key range
    if (last.sequence < bound_.seq) {
     abandoned_entries.push_back(temp);
     return kDeleteLast;
    }
    if (current.sequence >= bound_.seq) {
     return kAcceptEntry;
    }
   }
  }
 }

 return kAcceptEntry;
}

uint64_t BaselineMerger::MergeEntries() {
 // The filtering of arbitration is inside the merge process
 int entries_read_out;
 stream_merger_heap::entry_file_heap heap;
 for (auto &plain_file: input_plain_files) {
  std::string current_entry;
  entries_read_out = NextEntry(plain_file, &current_entry);

  if (entries_read_out == 0) {
   input_plain_files.erase(std::find(input_plain_files.begin(), input_plain_files.end(), plain_file));
   // doesn't sure it will work or not
  } else {
   // read out at least one entry from the files
   heap.emplace(current_entry, plain_file);
  }

 }
 // heap is built, and all files are added into the heap
 std::string result_block;
 PlainTable output_file(fileNameCreator->NextFileName(), false);
 std::string current_entry;
 ArbitrationAction action;
 std::string next_entry;
 while (!heap.empty()) {
  if (result_block.size() > BLOCK_SIZE) {
   AppendOutput(&output_file, result_block);
   result_block.clear();
  }
  auto heap_head = heap.top();
  current_entry = heap.top().first;
  action = Arbitration(current_entry);
//   result_block.append(heap.top().first);
  // after Arbitration, the last_entry is same as the current entry
  if (action == kAcceptEntry) {
   result_block.append(last_entry);
  }
  auto entries = NextEntry(heap_head.second, &next_entry);
  if (entries > 0) {
   heap.emplace(next_entry, heap_head.second);
  }
  heap.pop();
 }

 AppendOutput(&output_file, result_block);


 return 0;
}

uint64_t BaselineMerger::PrepareFiles() {
 for (auto file: input_files) {
  input_plain_files.push_back(dynamic_cast<PlainTable *> (file));
 }
 return 0;
}

uint64_t BaselineMerger::DoCompaction() {
 PrepareFiles();
 // do the merge and write out ouputs
 MergeEntries();
}


// the stream merger is the merger that will transport piece by piece, and wait for the results from FPGA
FPGA_Stream_Merger::FPGA_Stream_Merger(std::vector<std::string> input_files, TableFormat format,
                                       FileNameCreator *fileNameHandler,
                                       ssize_t file_window_size)
    : Merger(input_files, format, fileNameHandler) {
 uint block_num = file_window_size / BLOCK_SIZE;
 file_window_size = BLOCK_SIZE * block_num;
}

inline void TransportToFPGA(const char *buffer, uint64_t data_size) {
 // the function that send data buffer to the FPGA
 if (data_size == 0) {
  return;
 }
}

inline void WaitForFPGA() {}

inline void
InitialArbitrationCondition(FilterLogic arbitration, FilterArgs values, std::vector<std::string> *abandoned_entries) {
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
 // Init the compaction job on FPGA
 InitialArbitrationCondition(this->logic_, this->bound_, &abandoned_entries_for_FPGA);

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

uint64_t FPGA_Stream_Merger::MergeEntries() {
 return -1;
}
