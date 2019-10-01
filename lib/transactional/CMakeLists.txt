cmake_minimum_required (VERSION 3.5)

project(transactional)

add_library(transaction STATIC transactional.cpp)
target_include_directories(transaction PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(transaction PUBLIC sqlite3)
target_compile_features(transaction PUBLIC cxx_std_17)
