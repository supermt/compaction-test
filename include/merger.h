//
// Created by supermt on 8/11/22.
//

#ifndef COMPACTION_TEST_MERGER_H
#define COMPACTION_TEST_MERGER_H

#include <atomic>
#include "tables/table.h"

enum TableFormat : int {
  kGear = 0x0,
  kBlockBased,
  kPlain,
  kCuckoo
};

class FileNameCreator {
public:
  std::atomic<int> file_number;

  FileNameCreator();

  std::string NextFileName();
};

class Merger {
public:
  Merger(std::vector<std::string> input_fnames, TableFormat format);

  FileNameCreator fileNameCreator;

  std::vector<std::string> CreateInputFileNames(uint64_t number_of_input_files);

  Table *CreateFileFromName(std::string fname);

  uint64_t DoCompaction();

  uint64_t MergeEntries();

  uint64_t PrepareEntries();

  uint64_t WriteOutResult();

  std::vector<Table *> input_files;
  std::vector<Slice> result_keys;
  std::vector<Slice> result_values;
  TableFormat format;
};


#endif //COMPACTION_TEST_MERGER_H
