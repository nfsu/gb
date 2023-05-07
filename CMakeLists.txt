# nfsu/gb, a basic gameboy (color) emulator.
# Copyright (C) 2023 NFSU / Nielsbishere (Niels Brunekreef)
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program. If not, see https://github.com/NFSU/gb/blob/revival/LICENSE.
# Be aware that GPL3 requires closed source products to be GPL3 too if released to the public.
# To prevent this a separate license will have to be requested at contact@osomi.net for a premium;
# This is called dual licensing.

cmake_minimum_required(VERSION 3.13.0)
project(gb)

set_property(GLOBAL PROPERTY USE_FOLDERS true)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION true)
set(CMAKE_SUPPRESS_REGENERATION true)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED true)
set(CMAKE_C_EXTENSIONS false)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(MSVC)
	add_compile_options(/W4 /WX /MD /MP /GR /arch:AVX2 /D_CRT_SECURE_NO_WARNINGS /Gy)
else()
	add_compile_options(-Wall -Wextra -Werror -mavx2)
endif()

file(GLOB_RECURSE headers "inc/*.h")
file(GLOB_RECURSE sources "src/*.c")

add_library(
	gb
	STATIC
	${headers}
	${sources}
	CMakeLists.txt
)

set_target_properties(gb PROPERTIES FOLDER NFSU/gb)

add_subdirectory(core3)

target_include_directories(gb PUBLIC inc/emu)
target_link_libraries(gb PUBLIC OxC3_platforms)

file(GLOB_RECURSE ui "ui/*.c")

add_executable(
	gb_emu
	${ui}
	CMakeLists.txt
)

set_target_properties(gb_emu PROPERTIES FOLDER NFSU/gb/test)

target_link_libraries(gb_emu PUBLIC gb)

configure_icon(gb_emu "${CMAKE_CURRENT_SOURCE_DIR}/res/logo.ico")
configure_virtual_files(gb_emu)
