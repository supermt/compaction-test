//
// Created by supermt on 8/10/22.
//

#include "tables/gear_table_builder.h"

class GearBlock {
public:
  explicit GearBlock(int block_id) : num_of_entries(0), block_id_(block_id) {
  }

  int AddBlock(const std::vector<Slice> &keys, const std::vector<Slice> &values);

  int ComposeResults();

  std::string content_block;
  std::string key_content;
  std::string value_content;
  int num_of_entries;
  const int block_id_;
};

int GearBlock::AddBlock(const std::vector<Slice> &keys, const std::vector<Slice> &values) {
 assert(keys.size() == values.size());
 num_of_entries = keys.size();
 for (ssize_t i = 0; i < num_of_entries; i++) {
  key_content.append(keys[num_of_entries - i - 1].data(), keys[num_of_entries - i - 1].size());
  value_content.append(values[i].data(), values[i].size());
 }
 ComposeResults();
 return num_of_entries;
}

int GearBlock::ComposeResults() {
 uint32_t placeholder_length =
     BLOCK_SIZE - (HEADER_LENGTH +
                   key_content.size() + value_content.size());

 PutFixed32(&content_block, block_id_);
 PutFixed32(&content_block, num_of_entries);
 PutFixed32(&content_block, (uint32_t) key_content.size());
 PutFixed32(&content_block, (uint32_t) value_content.size());
 PutFixed32(&content_block, placeholder_length);

 // place holder
 PutFixed32(&content_block, 0);
 PutFixed32(&content_block, 0);
 PutFixed32(&content_block, 0);
 PutFixed32(&content_block, 0);
 PutFixed32(&content_block, 0);
 PutFixed32(&content_block, 0);
 PutFixed32(&content_block, 0);
 PutFixed32(&content_block, 0);
 PutFixed32(&content_block, 0);
 PutFixed32(&content_block, 0);
 PutFixed32(&content_block, 0);
 content_block.append(value_content);
 std::string placeholder(placeholder_length, 'h');
 content_block.append(placeholder);
 content_block.append(key_content);
 return content_block.size();
}

GearTable::GearTable(const std::string &fname, bool read)
    : Table(fname, read) {
 std::cout << "creating gear table" << std::endl;
}

int GearTable::ToDiskFormat(std::string *disk_format_buffer) {
 int block_id = 0;
 for (size_t i = 0; i < key_list.size(); i += MAX_ENTRY_IN_BLOCK) {
  assert(key_list.size() == value_list.size());
  size_t end_posi = i + MAX_ENTRY_IN_BLOCK > key_list.size() ? key_list.size() : i + MAX_ENTRY_IN_BLOCK;
  auto key_first = key_list.begin() + i;
  auto key_end = key_list.begin() + end_posi;
  std::vector<Slice> sub_key(key_first, key_end);

  auto value_first = value_list.begin() + i;
  auto value_end = value_list.begin() + end_posi;

  std::vector<Slice> sub_value(value_first, value_end);
  GearBlock temp(block_id);
  temp.AddBlock(sub_key, sub_value);
  disk_format_buffer->append(temp.content_block);
 }

 return disk_format_buffer->size();
}

int GearTable::FromDiskFormat(const Slice &data_pack, uint32_t *last_entry_count) {
 return 0;
}
