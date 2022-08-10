//
// Created by supermt on 8/10/22.
//

#ifndef COMPACTION_TEST_BLOCK_BASED_READER_H
#define COMPACTION_TEST_BLOCK_BASED_READER_H

#include "tables/table.h"

class BlockBasedTable : public Table {


public:

  int ToDiskFormat(std::string *disk_format_buffer) override;

  int FromDiskFormat(const Slice &data_pack, uint32_t *last_entry_count) override;

  explicit BlockBasedTable(std::string fname);

};


#endif //COMPACTION_TEST_BLOCK_BASED_READER_H
