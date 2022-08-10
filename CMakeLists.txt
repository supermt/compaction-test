cmake_minimum_required(VERSION 3.5.1)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules/")
project(compaction-test
        VERSION 1.1
        LANGUAGES CXX C ASM)

if (POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
endif ()

if (NOT CMAKE_BUILD_TYPE)
    if (EXISTS "${CMAKE_SOURCE_DIR}/.git")
        set(default_build_type "Debug")
    else ()
        set(default_build_type "RelWithDebInfo")
    endif ()
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING
            "Default BUILD_TYPE is ${default_build_type}" FORCE)
endif ()

find_program(CCACHE_FOUND ccache)
if (CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif (CCACHE_FOUND)

#--xuan
SET(XILINX_XRT "/opt/xilinx/xrt")
SET(VITIS_PLATFORM "xilinx_aws-vu9p-f1_shell-v04261818_201920_3")
SET(VITIS_PLATFORM_DIR "/home/sunny/aws-fpga/Vitis/aws_platform/xilinx_aws-vu9p-f1_shell-v04261818_201920_3")
SET(VITIS_PLATFORM_PATH "${VITIS_PLATFORM_DIR}/xilinx_aws-vu9p-f1_shell-v04261818_201920_3.xpfm")
add_definitions(-DVITIS_PLATFORM=${VITIS_PLATFORM} -D__USE_XOPEN2K8)

option(WITH_JEMALLOC "build with JeMalloc" OFF)
option(WITH_SNAPPY "build with SNAPPY" OFF)
option(WITH_LZ4 "build with lz4" OFF)
option(WITH_ZLIB "build with zlib" OFF)
option(WITH_ZSTD "build with zstd" OFF)
option(WITH_WINDOWS_UTF8_FILENAMES "use UTF8 as characterset for opening files, regardles of the system code page" OFF)
if (WITH_WINDOWS_UTF8_FILENAMES)
    add_definitions(-DROCKSDB_WINDOWS_UTF8_FILENAMES)
endif ()
# third-party/folly is only validated to work on Linux and Windows for now.
# So only turn it on there by default.
if (CMAKE_SYSTEM_NAME MATCHES "Linux|Windows")
    if (MSVC AND MSVC_VERSION LESS 1910)
        # Folly does not compile with MSVC older than VS2017
        option(WITH_FOLLY_DISTRIBUTED_MUTEX "build with folly::DistributedMutex" OFF)
    else ()
        option(WITH_FOLLY_DISTRIBUTED_MUTEX "build with folly::DistributedMutex" ON)
    endif ()
else ()
    option(WITH_FOLLY_DISTRIBUTED_MUTEX "build with folly::DistributedMutex" OFF)
endif ()

if (NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 11)
endif ()

include(CMakeDependentOption)
CMAKE_DEPENDENT_OPTION(WITH_GFLAGS "build with GFlags" ON
        "NOT MSVC;NOT MINGW" OFF)

string(TIMESTAMP TS "%Y/%m/%d %H:%M:%S" UTC)
set(GIT_DATE_TIME "${TS}" CACHE STRING "the time we first built compaction-test")

find_package(Git)

if (GIT_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    if (WIN32)
        execute_process(COMMAND $ENV{COMSPEC} /C ${GIT_EXECUTABLE} -C ${CMAKE_CURRENT_SOURCE_DIR} rev-parse HEAD OUTPUT_VARIABLE GIT_SHA)
    else ()
        execute_process(COMMAND ${GIT_EXECUTABLE} -C ${CMAKE_CURRENT_SOURCE_DIR} rev-parse HEAD OUTPUT_VARIABLE GIT_SHA)
    endif ()
else ()
    set(GIT_SHA 0)
endif ()

string(REGEX REPLACE "[^0-9a-f]+" "" GIT_SHA "${GIT_SHA}")


option(WITH_MD_LIBRARY "build with MD" ON)
if (WIN32 AND MSVC)
    if (WITH_MD_LIBRARY)
        set(RUNTIME_LIBRARY "MD")
    else ()
        set(RUNTIME_LIBRARY "MT")
    endif ()
endif ()

# add by jinghuan for cmake toolchain
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_INIT} -std=c++17 -pthread")

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi /nologo /EHsc /GS /Gd /GR /GF /fp:precise /Zc:wchar_t /Zc:forScope /errorReport:queue")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FC /d2Zi+ /W4 /wd4127 /wd4800 /wd4996 /wd4351 /wd4100 /wd4204 /wd4324")
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wextra -Wall")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wsign-compare -Wshadow -Wno-unused-parameter -Wno-unused-variable -Woverloaded-virtual -Wnon-virtual-dtor -Wno-missing-field-initializers -Wno-strict-aliasing")
    if (MINGW)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-format -fno-asynchronous-unwind-tables")
        add_definitions(-D_POSIX_C_SOURCE=1)
    endif ()
    if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
        include(CheckCXXCompilerFlag)
        CHECK_CXX_COMPILER_FLAG("-momit-leaf-frame-pointer" HAVE_OMIT_LEAF_FRAME_POINTER)
        if (HAVE_OMIT_LEAF_FRAME_POINTER)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -momit-leaf-frame-pointer")
        endif ()
    endif ()
endif ()

include(CheckCCompilerFlag)
if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc)64")
    CHECK_C_COMPILER_FLAG("-mcpu=power9" HAS_POWER9)
    if (HAS_POWER9)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=power9 -mtune=power9")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=power9 -mtune=power9")
    else ()
        CHECK_C_COMPILER_FLAG("-mcpu=power8" HAS_POWER8)
        if (HAS_POWER8)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=power8 -mtune=power8")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=power8 -mtune=power8")
        endif (HAS_POWER8)
    endif (HAS_POWER9)
    CHECK_C_COMPILER_FLAG("-maltivec" HAS_ALTIVEC)
    if (HAS_ALTIVEC)
        message(STATUS " HAS_ALTIVEC yes")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -maltivec")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -maltivec")
    endif (HAS_ALTIVEC)
endif (CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc)64")

if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|AARCH64")
    CHECK_C_COMPILER_FLAG("-march=armv8-a+crc+crypto" HAS_ARMV8_CRC)
    if (HAS_ARMV8_CRC)
        message(STATUS " HAS_ARMV8_CRC yes")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv8-a+crc+crypto -Wno-unused-function")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8-a+crc+crypto -Wno-unused-function")
    endif (HAS_ARMV8_CRC)
endif (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|AARCH64")

option(PORTABLE "build a portable binary" OFF)
option(FORCE_SSE42 "force building with SSE4.2, even when PORTABLE=ON" OFF)
if (PORTABLE)
    # MSVC does not need a separate compiler flag to enable SSE4.2; if nmmintrin.h
    # is available, it is available by default.
    if (FORCE_SSE42 AND NOT MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2 -mpclmul")
    endif ()
else ()
    if (MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2")
    else ()
        if (NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc)64" AND NOT HAS_ARMV8_CRC)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
        endif ()
    endif ()
endif ()

include(CheckCXXSourceCompiles)
if (NOT MSVC)
    set(CMAKE_REQUIRED_FLAGS "-msse4.2 -mpclmul")
endif ()

CHECK_CXX_SOURCE_COMPILES("
#include <cstdint>
#include <nmmintrin.h>
#include <wmmintrin.h>
int main() {
  volatile uint32_t x = _mm_crc32_u32(0, 0);
  const auto a = _mm_set_epi64x(0, 0);
  const auto b = _mm_set_epi64x(0, 0);
  const auto c = _mm_clmulepi64_si128(a, b, 0x00);
  auto d = _mm_cvtsi128_si64(c);
}
" HAVE_SSE42)
unset(CMAKE_REQUIRED_FLAGS)
if (HAVE_SSE42)
    add_definitions(-DHAVE_SSE42)
    add_definitions(-DHAVE_PCLMUL)
elseif (FORCE_SSE42)
    message(FATAL_ERROR "FORCE_SSE42=ON but unable to compile with SSE4.2 enabled")
endif ()

CHECK_CXX_SOURCE_COMPILES("
#if defined(_MSC_VER) && !defined(__thread)
#define __thread __declspec(thread)
#endif
int main() {
  static __thread int tls;
}
" HAVE_THREAD_LOCAL)
if (HAVE_THREAD_LOCAL)
    add_definitions(-DROCKSDB_SUPPORT_THREAD_LOCAL)
endif ()

option(FAIL_ON_WARNINGS "Treat compile warnings as errors" ON)
if (FAIL_ON_WARNINGS)
    if (MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")
    else () # assume GCC
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        #        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
    endif ()
endif ()

option(WITH_ASAN "build with ASAN" OFF)
if (WITH_ASAN)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
    if (WITH_JEMALLOC)
        message(FATAL "ASAN does not work well with JeMalloc")
    endif ()
endif ()

option(WITH_TSAN "build with TSAN" OFF)
if (WITH_TSAN)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread -pie")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -fPIC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread -fPIC")
    if (WITH_JEMALLOC)
        message(FATAL "TSAN does not work well with JeMalloc")
    endif ()
endif ()

option(WITH_UBSAN "build with UBSAN" OFF)
if (WITH_UBSAN)
    add_definitions(-DROCKSDB_UBSAN_RUN)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined")
    if (WITH_JEMALLOC)
        message(FATAL "UBSAN does not work well with JeMalloc")
    endif ()
endif ()

option(WITH_NUMA "build with NUMA policy support" OFF)
if (WITH_NUMA)
    find_package(NUMA REQUIRED)
    add_definitions(-DNUMA)
    include_directories(${NUMA_INCLUDE_DIR})
    list(APPEND THIRDPARTY_LIBS NUMA::NUMA)
endif ()

option(WITH_TBB "build with Threading Building Blocks (TBB)" OFF)
if (WITH_TBB)
    find_package(TBB REQUIRED)
    add_definitions(-DTBB)
    list(APPEND THIRDPARTY_LIBS TBB::TBB)
endif ()

option(WITH_DYNAMIC_EXTENSION "build with dynamic extension support" OFF)
if (NOT WITH_DYNAMIC_EXTENSION)
    add_definitions(-DROCKSDB_NO_DYNAMIC_EXTENSION)
endif ()

if (DEFINED USE_RTTI)
    if (USE_RTTI)
        message(STATUS "Enabling RTTI")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DROCKSDB_USE_RTTI")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DROCKSDB_USE_RTTI")
    else ()
        if (MSVC)
            message(STATUS "Disabling RTTI in Release builds. Always on in Debug.")
            set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DROCKSDB_USE_RTTI")
            set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GR-")
        else ()
            message(STATUS "Disabling RTTI in Release builds")
            set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-rtti")
            set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-rtti")
        endif ()
    endif ()
else ()
    message(STATUS "Enabling RTTI in Debug builds only (default)")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DROCKSDB_USE_RTTI")
    if (MSVC)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GR-")
    else ()
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-rtti")
    endif ()
endif ()

# Used to run CI build and tests so we can run faster
option(OPTDBG "Build optimized debug build with MSVC" OFF)
option(WITH_RUNTIME_DEBUG "build with debug version of runtime library" ON)
if (MSVC)
    if (OPTDBG)
        message(STATUS "Debug optimization is enabled")
        set(CMAKE_CXX_FLAGS_DEBUG "/Oxt")
    else ()
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /RTC1")

        # Minimal Build is deprecated after MSVC 2015
        if (MSVC_VERSION GREATER 1900)
            set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Gm-")
        else ()
            set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Gm")
        endif ()

    endif ()
    if (WITH_RUNTIME_DEBUG)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /${RUNTIME_LIBRARY}d")
    else ()
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /${RUNTIME_LIBRARY}")
    endif ()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Oxt /Zp8 /Gm- /Gy /${RUNTIME_LIBRARY}")

    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG")
endif ()

if (CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-builtin-memcmp")
endif ()

option(ROCKSDB_LITE "Build RocksDBLite version" OFF)
if (ROCKSDB_LITE)
    add_definitions(-DROCKSDB_LITE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -Os")
endif ()

if (CMAKE_SYSTEM_NAME MATCHES "Cygwin")
    add_definitions(-fno-builtin-memcmp -DCYGWIN)
elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    add_definitions(-DOS_MACOSX)
    if (CMAKE_SYSTEM_PROCESSOR MATCHES arm)
        add_definitions(-DIOS_CROSS_COMPILE -DROCKSDB_LITE)
        # no debug info for IOS, that will make our library big
        add_definitions(-DNDEBUG)
    endif ()
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_definitions(-DOS_LINUX)
elseif (CMAKE_SYSTEM_NAME MATCHES "SunOS")
    add_definitions(-DOS_SOLARIS)
elseif (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
    add_definitions(-DOS_FREEBSD)
elseif (CMAKE_SYSTEM_NAME MATCHES "NetBSD")
    add_definitions(-DOS_NETBSD)
elseif (CMAKE_SYSTEM_NAME MATCHES "OpenBSD")
    add_definitions(-DOS_OPENBSD)
elseif (CMAKE_SYSTEM_NAME MATCHES "DragonFly")
    add_definitions(-DOS_DRAGONFLYBSD)
elseif (CMAKE_SYSTEM_NAME MATCHES "Android")
    add_definitions(-DOS_ANDROID)
elseif (CMAKE_SYSTEM_NAME MATCHES "Windows")
    add_definitions(-DWIN32 -DOS_WIN -D_MBCS -DWIN64 -DNOMINMAX)
    if (MINGW)
        add_definitions(-D_WIN32_WINNT=_WIN32_WINNT_VISTA)
    endif ()
endif ()

if (NOT WIN32)
    add_definitions(-DROCKSDB_PLATFORM_POSIX -DROCKSDB_LIB_IO_POSIX)
endif ()

option(WITH_FALLOCATE "build with fallocate" ON)
if (WITH_FALLOCATE)
    CHECK_CXX_SOURCE_COMPILES("
#include <fcntl.h>
#include <linux/falloc.h>
int main() {
 int fd = open(\"/dev/null\", 0);
 fallocate(fd, FALLOC_FL_KEEP_SIZE, 0, 1024);
}
" HAVE_FALLOCATE)
    if (HAVE_FALLOCATE)
        add_definitions(-DROCKSDB_FALLOCATE_PRESENT)
    endif ()
endif ()

CHECK_CXX_SOURCE_COMPILES("
#include <fcntl.h>
int main() {
  int fd = open(\"/dev/null\", 0);
  sync_file_range(fd, 0, 1024, SYNC_FILE_RANGE_WRITE);
}
" HAVE_SYNC_FILE_RANGE_WRITE)
if (HAVE_SYNC_FILE_RANGE_WRITE)
    add_definitions(-DROCKSDB_RANGESYNC_PRESENT)
endif ()

CHECK_CXX_SOURCE_COMPILES("
#include <pthread.h>
int main() {
  (void) PTHREAD_MUTEX_ADAPTIVE_NP;
}
" HAVE_PTHREAD_MUTEX_ADAPTIVE_NP)
if (HAVE_PTHREAD_MUTEX_ADAPTIVE_NP)
    add_definitions(-DROCKSDB_PTHREAD_ADAPTIVE_MUTEX)
endif ()

include(CheckCXXSymbolExists)
check_cxx_symbol_exists(malloc_usable_size malloc.h HAVE_MALLOC_USABLE_SIZE)
if (HAVE_MALLOC_USABLE_SIZE)
    add_definitions(-DROCKSDB_MALLOC_USABLE_SIZE)
endif ()

check_cxx_symbol_exists(sched_getcpu sched.h HAVE_SCHED_GETCPU)
if (HAVE_SCHED_GETCPU)
    add_definitions(-DROCKSDB_SCHED_GETCPU_PRESENT)
endif ()

check_cxx_symbol_exists(getauxval auvx.h HAVE_AUXV_GETAUXVAL)
if (HAVE_AUXV_GETAUXVAL)
    add_definitions(-DROCKSDB_AUXV_GETAUXVAL_PRESENT)
endif ()

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/include)
#--xuan
#include_directories(
#        "${XILINX_XRT}/include/"
#        "/opt/Xilinx/Vitis_HLS/2021.2/include/"
#)
if (WITH_FOLLY_DISTRIBUTED_MUTEX)
    include_directories(${PROJECT_SOURCE_DIR}/third-party/folly)
endif ()
find_package(Threads REQUIRED)

# Main library source code

set(SOURCES

        utils/string_util.cc
        utils/cleanable.cc
        utils/coding.cc
        utils/slice.cc
        db_format/dbformat.cc
        db_format/on_board_fomat.cc
        include/merger.cc
        tables/block_based/block_based_table.cc
        tables/cuckoo_table/cuckoo_table.cc
        tables/gear_table/gear_table.cc
        tables/plain_table/plain_table.cc
        tables/table_builder.cc
        )

if (HAVE_SSE42 AND NOT MSVC)
    set_source_files_properties(
            util/crc32c.cc
            PROPERTIES COMPILE_FLAGS "-msse4.2 -mpclmul")
endif ()

if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc)64")
    list(APPEND SOURCES
            util/crc32c_ppc.c
            util/crc32c_ppc_asm.S)
endif (CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc)64")

if (HAS_ARMV8_CRC)
    list(APPEND SOURCES
            util/crc32c_arm64.cc)
endif (HAS_ARMV8_CRC)


list(APPEND SOURCES
        port/port_posix.cc
        )

set(ROCKSDB_STATIC_LIB rocksdb${ARTIFACT_SUFFIX})
set(ROCKSDB_SHARED_LIB rocksdb-shared${ARTIFACT_SUFFIX})

option(ROCKSDB_BUILD_SHARED "Build shared versions of the RocksDB libraries" ON)

set(SYSTEM_LIBS ${CMAKE_THREAD_LIBS_INIT})

add_library(${ROCKSDB_STATIC_LIB} STATIC ${SOURCES})
target_link_libraries(${ROCKSDB_STATIC_LIB}
        ${THIRDPARTY_LIBS} ${SYSTEM_LIBS})

if (ROCKSDB_BUILD_SHARED)
    add_library(${ROCKSDB_SHARED_LIB} SHARED ${SOURCES})
    target_link_libraries(${ROCKSDB_SHARED_LIB} PRIVATE
            ${THIRDPARTY_LIBS} ${SYSTEM_LIBS})

    if (WIN32)
        set_target_properties(${ROCKSDB_SHARED_LIB} PROPERTIES
                COMPILE_DEFINITIONS "ROCKSDB_DLL;ROCKSDB_LIBRARY_EXPORTS")
        if (MSVC)
            set_target_properties(${ROCKSDB_STATIC_LIB} PROPERTIES
                    COMPILE_FLAGS "/Fd${CMAKE_CFG_INTDIR}/${ROCKSDB_STATIC_LIB}.pdb")
            set_target_properties(${ROCKSDB_SHARED_LIB} PROPERTIES
                    COMPILE_FLAGS "/Fd${CMAKE_CFG_INTDIR}/${ROCKSDB_SHARED_LIB}.pdb")
        endif ()
    else ()
        set_target_properties(${ROCKSDB_SHARED_LIB} PROPERTIES
                LINKER_LANGUAGE CXX
                VERSION 1
                SOVERSION 1
                OUTPUT_NAME "compaction-test")
    endif ()
endif ()

if (ROCKSDB_BUILD_SHARED AND NOT WIN32)
    set(ROCKSDB_LIB ${ROCKSDB_SHARED_LIB})
else ()
    set(ROCKSDB_LIB ${ROCKSDB_STATIC_LIB})
endif ()

option(WITH_JNI "build with JNI" OFF)
# Tests are excluded from Release builds
CMAKE_DEPENDENT_OPTION(WITH_TESTS "build with tests" ON
        "CMAKE_BUILD_TYPE STREQUAL Debug" OFF)
option(WITH_BENCHMARK_TOOLS "build with benchmarks" ON)
option(WITH_CORE_TOOLS "build with ldb and sst_dump" ON)
option(WITH_TOOLS "build with tools" ON)

if (WITH_TESTS OR WITH_BENCHMARK_TOOLS OR WITH_TOOLS OR WITH_JNI OR JNI)
    include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/third-party/gtest-1.8.1/fused-src)
endif ()
if (WITH_JNI OR JNI)
    message(STATUS "JNI library is enabled")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/java)
else ()
    message(STATUS "JNI library is disabled")
endif ()

# Installation and packaging
if (WIN32)
    option(ROCKSDB_INSTALL_ON_WINDOWS "Enable install target on Windows" OFF)
endif ()
if (NOT WIN32 OR ROCKSDB_INSTALL_ON_WINDOWS)
    if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
            # Change default installation prefix on Linux to /usr
            set(CMAKE_INSTALL_PREFIX /usr CACHE PATH "Install path prefix, prepended onto install directories." FORCE)
        endif ()
    endif ()

    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    set(package_config_destination ${CMAKE_INSTALL_LIBDIR}/cmake/rocksdb)

    configure_package_config_file(
            ${CMAKE_CURRENT_LIST_DIR}/cmake/RocksDBConfig.cmake.in RocksDBConfig.cmake
            INSTALL_DESTINATION ${package_config_destination}
    )

    write_basic_package_version_file(
            RocksDBConfigVersion.cmake
            VERSION ${rocksdb_VERSION}
            COMPATIBILITY SameMajorVersion
    )

    install(DIRECTORY include/rocksdb COMPONENT devel DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

    install(DIRECTORY "${PROJECT_SOURCE_DIR}/cmake/modules" COMPONENT devel DESTINATION ${package_config_destination})

    install(
            TARGETS ${ROCKSDB_STATIC_LIB}
            EXPORT RocksDBTargets
            COMPONENT devel
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    )

    if (ROCKSDB_BUILD_SHARED)
        install(
                TARGETS ${ROCKSDB_SHARED_LIB}
                EXPORT RocksDBTargets
                COMPONENT runtime
                ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
                RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
                LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
                INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        )
    endif ()

    install(
            EXPORT RocksDBTargets
            COMPONENT devel
            DESTINATION ${package_config_destination}
            NAMESPACE RocksDB::
    )

    install(
            FILES
            ${CMAKE_CURRENT_BINARY_DIR}/RocksDBConfig.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/RocksDBConfigVersion.cmake
            COMPONENT devel
            DESTINATION ${package_config_destination}
    )
endif ()

option(WITH_ALL_TESTS "Build all test, rather than a small subset" ON)

if (WITH_BENCHMARK_TOOLS)

    LINK_DIRECTORIES(${XILINX_XRT}/lib/)    #--xuan
    add_executable(gear_bench
            tools/gear_bench_classes.cc
            tools/gear_bench_tool.cc
            )
    #    target_link_libraries(${ROCKSDB_LIB} INTERFACE xilinxopencl -Wl,-rpath-link,${XILINX_XRT}/lib)  #--xuan
    target_link_libraries(gear_bench
            ${ROCKSDB_LIB} gflags)
endif ()
