//
// Created by supermt on 8/10/22.
//

#ifndef COMPACTION_TEST_CUCKOO_TABLE_READER_H
#define COMPACTION_TEST_CUCKOO_TABLE_READER_H

#include "tables/table.h"

class CuckooTable final : public Table {


public:

  int ToDiskFormat(std::string *disk_format_buffer) override;

  int FromDiskFormat(const Slice &data_pack, uint32_t *last_entry_count) override;

  explicit CuckooTable(std::string fname);

};


#endif //COMPACTION_TEST_CUCKOO_TABLE_READER_H
