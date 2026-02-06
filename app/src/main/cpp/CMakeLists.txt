cmake_minimum_required(VERSION 3.18.1)

project("dwg-engine")

# Note: In a production environment, you would link against the real libredwg.a
# For this distributable demo, we compile a stub implementation.
# To use real library:
# 1. Provide libs/arm64-v8a/libredwg.a
# 2. Uncomment the add_library(libredwg...) block
# 3. Remove libredwg_stub.cpp from dwg-engine sources
# 4. Link libredwg in target_link_libraries

# add_library(libredwg STATIC IMPORTED)
# set_target_properties(libredwg PROPERTIES IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/../../../../libs/arm64-v8a/libredwg.a")

add_library( 
             dwg-engine
             SHARED
             native-lib.cpp
             libredwg_stub.cpp ) # Stub included here for immediate compilation

find_library( 
              log-lib
              log )

target_include_directories(dwg-engine PRIVATE "${CMAKE_SOURCE_DIR}/include")

target_link_libraries( 
                       dwg-engine
                       # libredwg 
                       ${log-lib} )
