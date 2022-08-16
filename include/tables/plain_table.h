//
// Created by supermt on 8/10/22.
//

#ifndef COMPACTION_TEST_PLAIN_TABLE_H
#define COMPACTION_TEST_PLAIN_TABLE_H

#include "tables/table.h"

class PlainTable : public Table {

public:

  int ToDiskFormat(std::string *disk_format_buffer) override;

  int FromDiskFormat(const Slice &data_pack, uint32_t *last_entry_count) override;

  explicit PlainTable(const std::string& fname, bool b);
};


#endif //COMPACTION_TEST_PLAIN_TABLE_H
