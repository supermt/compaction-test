//
// Created by jinghuan on 7/6/21.
//
#include <iostream>


#include <gflags/gflags.h>

#ifdef NUMA
#include <numa.h>
#include <numaif.h>
#endif
#ifndef OS_WIN

#include <unistd.h>

#endif

#include <fcntl.h>
#include <sys/types.h>
#include <filesystem>

#include <chrono>
#include <algorithm>
#include <atomic>
#include <cinttypes>
#include <condition_variable>
#include <cstddef>
#include <iterator>
#include <memory>
#include <mutex>
#include <queue>
#include <regex>
#include <thread>
#include <unordered_map>
#include <cstdlib>
#include "port/port.h"
#include "merger.h"
#include "gear_bench_classes.h"
#include "tables/block_based_table.h"
#include "tables/plain_table.h"
#include "tables/cuckoo_table.h"

using GFLAGS_NAMESPACE::ParseCommandLineFlags;
using GFLAGS_NAMESPACE::RegisterFlagValidator;
using GFLAGS_NAMESPACE::SetUsageMessage;

DEFINE_uint64(seed, 0, "random seed");
DEFINE_string(db_path, "/tmp/rocksdb/gear", "The database path");
DEFINE_string(table_format, "gear", "available formats: gear or normal");
DEFINE_int64(max_open_files, 100, "max_opened_files");
DEFINE_int64(bench_threads, 1, "number of working threads");

DEFINE_double(span_range, 1.0, "The overlapping range of ");
DEFINE_double(min_value, 0, "The min values of the key range");
DEFINE_uint64(entries_per_file, 10324400, "number of distinct entries");
DEFINE_uint64(distinct_num, FLAGS_entries_per_file * 8,
              "number of distinct entries");
DEFINE_string(benchmark, "baseline", "baseline merger");

// Key size settings.
DEFINE_int32(key_size, 8, "size of each user key");
DEFINE_int32(value_size, 10, "size of each value");
// DB column settings
SequenceNumber seq_list[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

static enum TableFormat StringToRepFactory(const char *ctype) {
 assert(ctype);

 if (!strcasecmp(ctype, "gear"))
  return kGear;
 else if (!strcasecmp(ctype, "block_based"))
  return kBlockBased;
 else if (!strcasecmp(ctype, "plain"))
  return kPlain;
 else if (!strcasecmp(ctype, "cuckoo"))
  return kCuckoo;
 fprintf(stdout, "Cannot parse memreptable %s\n", ctype);
 return kGear;
}

static enum TableFormat FLAGS_table_format_enum;

inline Table *CreateTablePtr(const std::string &target_file_name) {
 Table *current_file;
 switch (FLAGS_table_format_enum) {
  case kGear:
   current_file = new GearTable(target_file_name);
   break;
  case kBlockBased:
   current_file = new BlockBasedTable(target_file_name, false);
   break;
  case kPlain:
   current_file = new PlainTable(target_file_name, false);
   break;
  case kCuckoo:
   current_file = new CuckooTable(target_file_name, false);
   break;
 }
 return current_file;
}

void generate_files(uint64_t start_key, uint64_t range, Random64 *rand,
                    FileNameCreator *file_name_handler,
                    std::vector<std::string> *input_file_names) {
 KeyGenerator key_gen(rand, INCREMENTAL_RANDOM, range, FLAGS_seed,
                      KEY_CONTENT_LENGTH,
                      start_key);
 uint64_t generated_entries = 0;
 // create the files
 Random32 seq_rand(FLAGS_seed);
 std::cout << "creating files from: " << start_key << std::endl;

 while (true) {
  std::string target_file_name = file_name_handler->NextFileName();
  input_file_names->push_back(target_file_name);

  Table *current_file = CreateTablePtr(target_file_name);
  for (uint64_t i = 0; i < FLAGS_entries_per_file; i++) {
   if (generated_entries >= range) {
    break;
   }
   //   Slice value = Slice("1234554321", VALUE_LENGTH);
   InternalKey ikey;
   ikey.Set(key_gen.NextString(), kMaxSequenceNumber - 1, kTypeValue);
   current_file->key_list.push_back(ikey.Encode());
   current_file->value_list.emplace_back("v1v2v3v4v5", 10);
   generated_entries++;
  } //end for
  std::string disk_format;
  current_file->ToDiskFormat(&disk_format);
  current_file->WriteToDisk(disk_format);
  delete current_file;

  if (generated_entries >= range - 1) {
   break;
  }
 }//end while
}

std::vector<std::string> split(const std::string &s, char delim) {
 std::vector<std::string> result;
 std::stringstream ss(s);
 std::string item;

 while (getline(ss, item, delim)) {
  result.push_back(item);
 }

 return result;
}

void FPGAMerger() {
 std::cout << "start" << std::endl;

 std::filesystem::create_directories(FLAGS_db_path);
 FileNameCreator file_name_handler(FLAGS_db_path);
 Random64 rand(FLAGS_seed);

 std::atomic_uint64_t file_bytes;


 auto start = std::chrono::steady_clock::now();

 std::vector<uint64_t> start_posi(FLAGS_bench_threads);
 uint64_t range_step = FLAGS_distinct_num / FLAGS_bench_threads;
 std::vector<std::string> input_files;
 FLAGS_table_format_enum = StringToRepFactory(FLAGS_table_format.c_str());
 // create the files
 for (int i = 0; i < FLAGS_bench_threads; i++) {
  start_posi.push_back(range_step);
//  generate_files(start_posi[i], range_step, &rand, &file_name_handler, &input_files);
  std::thread t(generate_files, start_posi[i], range_step, &rand,
                &file_name_handler, &input_files);
  t.join();
 }

 auto end = std::chrono::steady_clock::now();
 std::chrono::duration<double> elapsed_seconds = end - start;
 std::cout << "Secs used: " << elapsed_seconds.count() << std::endl;
}


void BaselineMergeTask(bool merge_only) {
 std::cout << "start" << std::endl;
 std::string cmd = "rm -rf " + FLAGS_db_path;

 if (!merge_only) {
  system(cmd.c_str());
  std::filesystem::create_directories(FLAGS_db_path);
 }
 cmd = "rm -rf " + FLAGS_db_path + "/results";
 system(cmd.c_str());
 std::filesystem::create_directories(FLAGS_db_path + "/results");

 FileNameCreator file_name_handler(FLAGS_db_path);
 Random64 rand(FLAGS_seed);

 std::atomic_uint64_t file_bytes;


 auto start = std::chrono::steady_clock::now();
 auto end = std::chrono::steady_clock::now();
 std::chrono::duration<double> elapsed_seconds = end - start;


 std::vector<uint64_t> start_posi(FLAGS_bench_threads);
 uint64_t range_step = FLAGS_distinct_num / FLAGS_bench_threads;
 std::vector<std::string> input_files;
 FLAGS_table_format_enum = kPlain;
 if (!merge_only) {
// create the files
  for (int i = 0; i < FLAGS_bench_threads; i++) {
   start_posi.push_back(range_step);
//  generate_files(start_posi[i], range_step, &rand, &file_name_handler, &input_files);
   std::thread t(generate_files, start_posi[i], range_step, &rand,
                 &file_name_handler, &input_files);
   t.join();
   end = std::chrono::steady_clock::now();

   std::chrono::duration<double> elapsed_seconds = end - start;
   std::cout << "File Creation Time(sec): " << elapsed_seconds.count()
             << std::endl;
  }
 } else {
  // get the name only
  for (int i = 0; i < FLAGS_distinct_num / FLAGS_entries_per_file; i++)
   input_files.push_back(file_name_handler.NextFileName());
 }
 FileNameCreator output_files(FLAGS_db_path + "/results");


 BaselineMerger merger(input_files, &output_files);
 merger.GenerateFilterArgs(kRemoveRedundant, FilterArgs());
 start = std::chrono::steady_clock::now();
 merger.DoCompaction();
 end = std::chrono::steady_clock::now();
 elapsed_seconds = end - start;
 std::cout << "Merge Time (sec): " << elapsed_seconds.count() << std::endl;
}


int main(int argc, char **argv) {
 ParseCommandLineFlags(&argc, &argv, true);

 auto benchmarks = split(FLAGS_benchmark, ',');
 for (const auto &benchmark: benchmarks) {
  if (benchmark == "gear") { FPGAMerger(); }
  else if (benchmark == "baseline_full") { BaselineMergeTask(false); }
  else if (benchmark == "baseline") { BaselineMergeTask(true); }
 }


}