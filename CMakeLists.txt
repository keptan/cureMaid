cmake_minimum_required (VERSION 3.5)

project(cuteMaid)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(lib/transactional)

add_executable(a.out
	src/main.cpp
) 

target_link_libraries(a.out
	PRIVATE
		transaction
)

target_compile_features(a.out PUBLIC cxx_std_17)
