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
 PrepareFiles();
 MergeEntries();
 DoFilter();
 WriteOutResult();
 return 0;
}


Merger::Merger(std::vector<std::string> input_fnames, TableFormat tableFormat,
               FileNameCreator *file_name_handler) {
 this->fileNameCreator_ = file_name_handler;
 format = tableFormat;
 for (auto input_fname: input_fnames) {
  input_files.push_back(CreateFilePointerFromName(input_fname));
 }
}

uint64_t Merger::MergeEntries() {
 result_keys.clear();
 result_values.clear();
 // we merge it by a min heap
 merge_by_min_heap(input_files, &pq);
 return 0;
}

Table *Merger::CreateFilePointerFromName(std::string fname) const {
 Table *temp_ptr;
 switch (format) {
  case kGear:
   temp_ptr = new GearTable(fname, true);
   break;
  case kBlockBased:
   temp_ptr = new BlockBasedTable(fname, true);
   break;
  case kPlain:
   temp_ptr = new PlainTable(fname, true);
   break;
  case kCuckoo:
   temp_ptr = new CuckooTable(fname, true);
   break;
 }
 return temp_ptr;
}

uint64_t Merger::DoFilter() {

 while (!pq.empty()) {
  auto current_pair = pq.top();
  auto last_pair = result_keys.back();
  switch (logic_) {
   case kRemoveRedundant:
    // save only the last key
    DropRedundantKeys(current_pair, last_pair);
    break;
   case kDeletePrefix:
    // remove all files with certain prefix
    DropWithPrefix(current_pair);
    break;
   case kDeleteVersion:
    PickUniqueVersion(current_pair, last_pair);
    // remove all entries when version number is smaller
    break;
  }
  pq.pop();
 }

 return 0;
}

void Merger::GenerateFilterArgs(FilterLogic judgement_logic,
                                FilterArgs judement_arg) {
 logic_ = judgement_logic;
 bound_ = judement_arg;
}

void Merger::DropRedundantKeys(std::pair<Slice *, Slice *> current_pair,
                               Slice last_key) {

 ParsedInternalKey parsed_current;
 ParsedInternalKey parsed_last;
 ParseInternalKey(*current_pair.first, &parsed_current);
 ParseInternalKey(last_key, &parsed_current);

 if (parsed_current.user_key == parsed_last.user_key) {
  abandoned_values.emplace_back(result_keys.back(), result_values.back());
  result_keys.pop_back();
  result_values.pop_back();
 }
 result_keys.push_back(*current_pair.first);
 result_values.push_back(*current_pair.second);
}

void Merger::DropWithPrefix(std::pair<Slice *, Slice *> current_pair) {
 ParsedInternalKey parsed_current;
 ParseInternalKey(*current_pair.first, &parsed_current);
 if (parsed_current.user_key.starts_with(bound_.prefix)) {
  abandoned_values.emplace_back(*current_pair.first, *current_pair.second);
 }

}

void Merger::PickUniqueVersion(std::pair<Slice *, Slice *> current_pair,
                               Slice last_key) {

 ParsedInternalKey parsed_current;
 ParsedInternalKey parsed_last;
 ParseInternalKey(*current_pair.first, &parsed_current);
 ParseInternalKey(last_key, &parsed_current);


 if (parsed_current.user_key.starts_with(bound_.prefix) &&
     parsed_current.sequence <= bound_.seq) {

  if (redundant_user_keys.back().first == parsed_current.user_key) {
   abandoned_values.emplace_back(redundant_user_keys.back());
   redundant_user_keys.pop_back();
  }
  redundant_user_keys.emplace_back(result_keys.back(), result_values.back());


  //  abandoned_values.emplace_back(result_keys.back(), result_values.back());
  result_keys.pop_back();
  result_values.pop_back();
 }
 result_keys.push_back(*current_pair.first);
 result_values.push_back(*current_pair.second);

}

std::string FileNameCreator::NextFileName() {
 std::ostringstream ostr;
 ostr << base_dir << "/" << std::setfill('0') << std::setw(8)
      << file_number.fetch_add(1) << ".sst";
 return ostr.str();
}
