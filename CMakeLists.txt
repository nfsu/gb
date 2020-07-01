cmake_minimum_required(VERSION 3.13.0)
project(gb)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_SUPPRESS_REGENERATION true)

add_subdirectory(emu)
add_subdirectory(igx)

include_directories(include)
include_directories(emu/include)
include_directories(emu/core2/include)
include_directories(igx/include)
include_directories(igx/ignis/include)

file(GLOB_RECURSE gbSrc
	"include/*.hpp"
	"include/*.inc.hpp"
	"src/*.cpp"
)

add_executable(
	gb
	${gbSrc}
	CMakeLists.txt
)

set_property(TARGET gb PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/~)

target_link_libraries(gb ocore ignis igx)

if(MSVC)
    target_compile_options(gb PRIVATE /W4 /WX /MD /MP /wd26812 /wd4201 /EHsc /GR)
else()
    target_compile_options(gb PRIVATE -Wall -Wpedantic -Wextra -Werror)
endif()