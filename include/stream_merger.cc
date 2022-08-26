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

BaselineMerger::BaselineMerger(std::vector<std::string> input_files,
                               FileNameCreator *fileNameCreator) : Merger(
    input_files, kPlain, fileNameCreator) {
 this->fileNameCreator_ = fileNameCreator;
}

inline uint64_t NextEntry(PlainTable *table, Slice &entry) {
// int readed_bytes = table->ReadFromDisk(entry, FULL_KEY_LENGTH + VALUE_LENGTH);
// return readed_bytes / (FULL_KEY_LENGTH + VALUE_LENGTH);
 entry = Slice(table->file_content.data() +
               table->readed_entries * (FULL_KEY_LENGTH + VALUE_LENGTH),
               FULL_KEY_LENGTH + VALUE_LENGTH);
 table->readed_entries++;
 return table->readed_entries * (FULL_KEY_LENGTH + VALUE_LENGTH);
}


namespace stream_merger_heap {
  typedef std::pair<Slice, int> entry_file_pair;

  class KeyComparor {
  public:
    int operator()(const entry_file_pair &p1, const entry_file_pair &p2) {

     ParsedInternalKey p1_key;
     ParsedInternalKey p2_key;
     ParseInternalKey(Slice(p1.first.data(), FULL_KEY_LENGTH), &p1_key);
     ParseInternalKey(Slice(p1.first.data(), FULL_KEY_LENGTH), &p2_key);

     int r = p1_key.user_key.compare(p2_key.user_key);
     if (r == 0) {
      if (p1_key.sequence > p2_key.sequence) {
       r = -1;
      } else if (p1_key.sequence < p2_key.sequence) {
       r = +1;
      } else if (p1_key.type > p2_key.type) {
       r = -1;
      } else if (p1_key.type < p2_key.type) {
       r = +1;
      }
     }

     return r;
    }
  };

  typedef std::priority_queue<entry_file_pair, std::vector<entry_file_pair>, KeyComparor> entry_file_heap;

};
namespace slice_merge_heap {
  class KeyComparor {
  public:
    int operator()(const Slice &p1, const Slice &p2) {

     ParsedInternalKey p1_key;
     ParsedInternalKey p2_key;
     ParseInternalKey(Slice(p1.data(), FULL_KEY_LENGTH), &p1_key);
     ParseInternalKey(Slice(p2.data(), FULL_KEY_LENGTH), &p2_key);

     int r = p1_key.user_key.compare(p2_key.user_key);
     if (r == 0) {
      if (p1_key.sequence > p2_key.sequence) {
       r = -1;
      } else if (p1_key.sequence < p2_key.sequence) {
       r = +1;
      } else if (p1_key.type > p2_key.type) {
       r = -1;
      } else if (p1_key.type < p2_key.type) {
       r = +1;
      }
     }

     return r;
    }
  };

  typedef std::priority_queue<Slice, std::vector<Slice>, KeyComparor> entry_heap;

};


inline Slice
EncodeDeletedKey(ParsedInternalKey &deleted_key, std::string &last_entry) {
 std::string last_key = InternalKey(deleted_key.user_key, deleted_key.sequence,
                                    kTypeDeletion).Encode().ToString();

 last_key.append(last_entry.data() + FULL_KEY_LENGTH,
                 VALUE_LENGTH);
 return {last_key.data(), FULL_KEY_LENGTH + VALUE_LENGTH};
}

BaselineMerger::ArbitrationAction
BaselineMerger::Arbitration(const Slice &current_entry,
                            Slice &result_buffer) {
 if (current_entry.empty() || last_entry.empty()) return kAcceptEntry;
 ParsedInternalKey last;
 ParsedInternalKey current;
 ParseInternalKey(Slice(last_entry.data(), FULL_KEY_LENGTH), &last);
 ParseInternalKey(Slice(current_entry.data(), FULL_KEY_LENGTH), &current);

 switch (logic_) {
  case kRemoveRedundant: {
   if (last.user_key == current.user_key) {
    abandoned_entries.push_back(last_entry);
    result_buffer = EncodeDeletedKey(last, last_entry);
//        Slice(last_key.data(), FULL_KEY_LENGTH + VALUE_LENGTH);
    return kDeleteLast;
   } else {
    result_buffer = last_entry;
    return kAcceptEntry;
   }
  }
  case kDeletePrefix: {
   // the arbitration only determines the existing of last key
   if (last.user_key.starts_with(bound_.prefix)) {
    abandoned_entries.push_back(last_entry);
    result_buffer = EncodeDeletedKey(last, last_entry);
    return kDeleteLast;
   } else {
    result_buffer = last_entry;
    return kAcceptEntry;
   }
  }
  case kDeleteVersion: {
   if (last.user_key.starts_with(bound_.prefix)) {
    // last key is in the key range
    if (last.user_key == current.user_key) {
     // last key is redundant
     if (last.sequence < bound_.seq) {
      // and the last key is in the invalid sequence
      abandoned_entries.push_back(last_entry);
      result_buffer = EncodeDeletedKey(last, last_entry);
      return kDeleteLast;
     } // not in the invalid sequence
    }// not repeating use key
   }//not in the prefix
   result_buffer = last_entry;
   return kAcceptEntry;
  }
 }

 return kAcceptEntry;
}

uint64_t BaselineMerger::MergeEntries() {
 // The filtering of arbitration is inside the merge process
 int entries_read_out;
 stream_merger_heap::entry_file_heap heap;
 int fid = 0;
 auto start = std::chrono::steady_clock::now();
 for (auto &plain_file: input_plain_files) {
  Slice current_entry;
  std::cout << plain_file->file_name << std::endl;
  plain_file->file_size = plain_file->ReadFromDisk(plain_file->file_content,
                                                   512 * 1024 * 1024ul);

 }
 auto end = std::chrono::steady_clock::now();

 std::chrono::duration<double> elapsed_seconds = end - start;
 std::cout << "File Loading Time (sec): " << elapsed_seconds.count()
           << std::endl;

 std::cout << "All files loaded" << std::endl;
 start = std::chrono::steady_clock::now();
 for (auto file: input_plain_files) {
  std::reverse(file->file_content.begin(), file->file_content.end());
  std::reverse(file->file_content.begin(), file->file_content.end());
 }

 end = std::chrono::steady_clock::now();
 elapsed_seconds = end - start;
 std::cout << "Format transform Time (Sec):" << elapsed_seconds.count()
           << std::endl;

 start = std::chrono::steady_clock::now();

 slice_merge_heap::entry_heap merger;
 Slice temp;
 uint64_t added_entries = 0;
 for (auto file: input_plain_files) {
  uint64_t readed_len = 0;
  while (readed_len < file->file_size) {
   readed_len = NextEntry(file, temp);
   merger.emplace(temp);
   added_entries++;
   if (added_entries % 1000000 == 0) {
    std::cout << added_entries << std::endl;
   }
  }
 }

 end = std::chrono::steady_clock::now();
 elapsed_seconds = end - start;
 std::cout << "Merge Sort (sec): "
           << elapsed_seconds.count()
           << std::endl;
 start = std::chrono::steady_clock::now();
 while (!merger.empty()) {
  merger.top();
  merger.pop();
 }
 end = std::chrono::steady_clock::now();
 elapsed_seconds = end - start;
 std::cout << "Merge & Arbitration By Heap Time (sec): "
           << elapsed_seconds.count()
           << std::endl;

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
FPGA_Stream_Merger::FPGA_Stream_Merger(std::vector<std::string> input_files,
                                       TableFormat format,
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
InitialArbitrationCondition(FilterLogic arbitration, FilterArgs values,
                            std::vector<std::string> *abandoned_entries) {
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
 InitialArbitrationCondition(this->logic_, this->bound_,
                             &abandoned_entries_for_FPGA);

 while (read_bytes != 0) {
  read_bytes = 0;
  buffer.clear();
  for (ssize_t fid = 0; fid < input_files.size(); fid++) {
   std::string read_buffer;
   int current_ouput = input_files[fid]->ReadFromDisk(read_buffer,
                                                      file_window_size);
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
