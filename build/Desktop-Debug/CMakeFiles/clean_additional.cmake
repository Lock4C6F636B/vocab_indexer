# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/vocab_indexer_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/vocab_indexer_autogen.dir/ParseCache.txt"
  "vocab_indexer_autogen"
  )
endif()
