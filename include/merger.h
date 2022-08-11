//
// Created by supermt on 8/11/22.
//

#ifndef COMPACTION_TEST_MERGER_H
#define COMPACTION_TEST_MERGER_H

#include <atomic>
#include <utility>
#include "tables/table.h"

enum FilterLogic : int {
  kRemoveRedundant = 0x0,
  kDeletePrefix,
  kDeleteVersion,
};

enum TableFormat : int {
  kGear = 0x0,
  kBlockBased,
  kPlain,
  kCuckoo
};

class FileNameCreator {
public:
  std::atomic<int> file_number;
  const std::string base_dir;

  FileNameCreator(std::string dir_name) : file_number(0), base_dir(std::move(dir_name)) {};

  std::string NextFileName();
};

struct FilterArgs {
  SequenceNumber seq;
  Slice prefix;
};

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

class Merger {
public:
  Merger(std::vector<std::string> input_fnames, TableFormat format, FileNameCreator *file_name_handler);

  FileNameCreator *fileNameCreator;

  std::vector<std::string> CreateInputFileNames(uint64_t number_of_input_files) const;

  Table *CreateFilePointerFromName(std::string fname) const;

  uint64_t DoCompaction();

  void GenerateFilterArgs(FilterLogic judgement_logic, FilterArgs judement_arg);

  void DropRedundantKeys(std::pair<Slice *, Slice *> pair, Slice slice);

  void DropWithPrefix(std::pair<Slice *, Slice *> pair);

  uint64_t MergeEntries();

  uint64_t PrepareEntries();

  uint64_t WriteOutResult();

  uint64_t DoFilter();

  std::vector<Table *> input_files;
  std::vector<Slice> result_keys;
  std::vector<Slice> result_values;
  std::vector<std::pair<Slice, Slice>> abandoned_values;

  std::vector<Table *> output_files;
  std::priority_queue<heap_merger::kv_pair, std::vector<heap_merger::kv_pair>, heap_merger::KeyComparor> pq;
  FilterLogic logic_;
  FilterArgs bound_;

  TableFormat format;

  void PickUniqueVersion(std::pair<Slice *, Slice *> current_pair, Slice last_key);
};


#endif //COMPACTION_TEST_MERGER_H
