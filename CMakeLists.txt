cmake_minimum_required (VERSION 3.0)

project(felix-long-readout-tools CXX)

add_executable(dumpfile-to-text dumpfile-to-text.cpp PdspChannelMapService.cpp)

add_executable(check-timestamps check-timestamps.cpp)

add_executable(extract-adcs extract_adcs_from_timestamp.cpp)

add_executable(dumpfile-to-33b dumpfile-to-33b.cpp PdspChannelMapService.cpp)