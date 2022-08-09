//
// Created by jinghuan on 7/6/21.
//
#include <iostream>

#include "gear_bench_classes.h"

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

#include "port/port.h"

using GFLAGS_NAMESPACE::ParseCommandLineFlags;
using GFLAGS_NAMESPACE::RegisterFlagValidator;
using GFLAGS_NAMESPACE::SetUsageMessage;
//--db=/media/jinghuan/nvme/gear_bench_test
//    --distinct_num=1000000
//        --write_buffer_size=500000
//            --benchmark=generate,merge
//            --use_existing_data=False
//                --span_range=1
//                    --l2_small_tree_num=2
//                        --max_background_compactions=4
//                            --target_file_size_base=500000
DEFINE_uint64(seed, 0, "random seed");
DEFINE_bool(statistics, false,
            "print out the detailed statistics after execution");
DEFINE_string(benchmark, "validate,generate,merge",
              "available values: "
              "[\'generate,merge\'] for generate and merge,\n"
              " \'generate\' for creat l2 big tree only \n"
              "\'merge\' for generate a l2 small tree, and trigger a L2 All In "
              "One Merge \n"
              "Please ensure when there is a merge operation in the benchmark, "
              "the use_existing_data is triggered");
// directory settings.
DEFINE_bool(use_exist_db, false, "Use the existing database or not");
DEFINE_bool(reload_keys, false, "Use the existing database or not");
DEFINE_bool(delete_new_files, true, "Delete L2 small tree after bench");
DEFINE_string(db_path, "/tmp/rocksdb/gear", "The database path");
DEFINE_string(table_format, "gear", "available formats: gear or normal");
DEFINE_int64(max_open_files, 100, "max_opened_files");
DEFINE_int64(bench_threads, 1, "number of working threads");

// key range settings.
DEFINE_int64(duration, 0, "Duration of Fill workloads");
DEFINE_double(span_range, 1.0, "The overlapping range of ");
DEFINE_double(min_value, 0, "The min values of the key range");
DEFINE_uint64(distinct_num, 1500000, "number of distinct entries");
DEFINE_uint64(existing_entries, 8000000000,
              "The number of entries inside existing database, this option "
              "will be ignored while use_existing_data is triggered");
DEFINE_uint64(l2_small_tree_num, 2, "Num of SST files in L2 Small tree");

// Key size settings.
DEFINE_int32(key_size, 8, "size of each user key");
DEFINE_int32(value_size, 10, "size of each value");
// DB column settings
DEFINE_int32(max_background_compactions, 1,
             "Number of concurrent threads to run.");
DEFINE_uint64(write_buffer_size, 312 * 800,
              "Size of Memtable, each flush will directly create a l2 small "
              "tree spanning in the entire key space");
DEFINE_uint64(target_file_size_base, 312 * 800,
              "Size of Memtable, each flush will directly create a l2 small "
              "tree spanning in the entire key space");
DEFINE_uint64(max_compaction_bytes, 50000000000,
              "max compaction, too small value will cut the SSTables into very "
              "small pieces.");
// DEFINE_int64(report_interval_seconds, 0,
//               "If greater than zero, it will write simple stats in CVS format
//               " "to --report_file every N seconds");
//
//  DEFINE_string(report_file, "report.csv",
//                "Filename where some simple stats are reported to (if "
//           c     "--report_interval_seconds is bigger than 0)");
DEFINE_string(index_dir_prefix, "index", "the index directory");
DEFINE_bool(print_data, false, "print out the keys with in HEX mode");

DEFINE_string(injection_sst_dir, "sst_files", "place to load the SST files");


int main(int argc, char **argv) {
 ParseCommandLineFlags(&argc, &argv, true);
 std::cout << "start" << std::endl;
}