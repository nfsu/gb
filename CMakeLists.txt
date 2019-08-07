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

include_directories(include)
include_directories(emu/include)
include_directories(emu/core2/include)

file(GLOB_RECURSE gbSrc
	"include/*.hpp"
	"src/*.cpp"
)

add_executable(
	gb
	${gbSrc}
)

target_link_libraries(gb ocore)

if(MSVC)
    target_compile_options(gb PRIVATE /W4 /WX /MD /MP /wd4201 /Ob2)
else()
    target_compile_options(gb PRIVATE -Wall -Wextra -pedantic -Werror)
endif()