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

uint64_t Merger::DoCompaction() {
 PrepareEntries();
 MergeEntries();
 WriteOutResult();
 return 0;
}

namespace heap_merger {
  typedef std::pair<Slice *, Slice *> kv_pair;

  class KeyComparor {
  public:
    int operator()(const kv_pair &p1, const kv_pair &p2) {

     ParsedInternalKey *p1_key = new ParsedInternalKey();
     ParsedInternalKey *p2_key = new ParsedInternalKey();
     ParseInternalKey(*p1.first, p1_key);
     ParseInternalKey(*p2.first, p2_key);

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

  inline void
  merge_by_min_heap(std::vector<Table *> files, std::priority_queue<kv_pair, std::vector<kv_pair>, KeyComparor> *pq) {
   for (auto file: files) {
    for (size_t i = 0; i < file->key_list.size(); i++) {
     pq->emplace(std::make_pair(file->key_list.data() + i, file->value_list.data() + i));
    }
   }

  }
};

Merger::Merger(std::vector<std::string> input_fnames, TableFormat tableFormat) {
 format = tableFormat;
 for (auto input_fname: input_fnames) {
  input_files.push_back(CreateFileFromName(input_fname));
 }
}

uint64_t Merger::MergeEntries() {
 result_keys.clear();
 result_values.clear();
 // we merge it by a min heap
 using namespace heap_merger;
 std::priority_queue<kv_pair, std::vector<kv_pair>, KeyComparor> pq;
 merge_by_min_heap(input_files, &pq);
 while (!pq.empty()) {
  auto key_value = pq.top();
  result_keys.push_back(*key_value.first);
  result_values.push_back(*key_value.second);
  pq.pop();
 }
 return 0;
}

uint64_t Merger::PrepareEntries() {
 size_t total_entries = 0;
 for (auto file: input_files) {
  std::string file_content;
  uint64_t last_entry_num;
  file->ReadFromDisk(&file_content, last_entry_num);
  Slice disk_data_pack(file_content.data(), file_content.size());
  // fill the key/value array
  file->FromDiskFormat(disk_data_pack, reinterpret_cast<uint32_t *>(&last_entry_num));
  total_entries += file->key_list.size();
 }

 return total_entries;
}

uint64_t Merger::WriteOutResult() {
 auto data_pack = new std::vector<OnBoardBlock>();
 ComposeOnBoardBlock(result_keys, result_values, data_pack);

 for (size_t i = 0; i < this->result_keys.size(); i += ENTRIES_PER_FILE) {
  std::string temp;
  size_t block_num = ENTRIES_PER_FILE + i > result_keys.size() ? result_keys.size() - i : ENTRIES_PER_FILE;
  for (size_t j = 0; j < block_num; j++) {
   temp.append(data_pack->at(j).content_block);
  }
  Table *temp_ptr = CreateFileFromName(fileNameCreator.NextFileName());
  uint32_t last_entry_count = 0;
  temp_ptr->FromOnBoardBlocks(Slice(temp), &last_entry_count);
 }

 return 0;
}

std::vector<std::string> Merger::CreateInputFileNames(uint64_t number_of_input_files) {
 std::vector<std::string> results(number_of_input_files);
 for (auto result: results) {
  result = fileNameCreator.NextFileName();
 }
 return results;
}

Table *Merger::CreateFileFromName(std::string fname) {
 Table *temp_ptr;
 switch (format) {
  case kGear:
   temp_ptr = new GearTableBuilder(fname);
   break;
  case kBlockBased:
   temp_ptr = new BlockBasedTable(fname);
   break;
  case kPlain:
   temp_ptr = new PlainTable(fname);
   break;
  case kCuckoo:
   temp_ptr = new CuckooTable(fname);
   break;
 }
 return temp_ptr;
}

FileNameCreator::FileNameCreator() : file_number(0) {

}

std::string FileNameCreator::NextFileName() {
 std::ostringstream ostr;
 ostr << std::setfill('0') << std::setw(8) << file_number.fetch_add(1) << ".sst";
 return ostr.str();
}
