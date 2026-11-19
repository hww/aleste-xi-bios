# alestelx: emulador del Aleste LX (modo nativo) para Windows y Linux
#   cmake -S . -B build && cmake --build build
# Necesita SDL2. En Windows: SDL2 de vcpkg (cmake --preset default, con
# VCPKG_ROOT; ver CMakePresets.json) o los "development libraries" de
# libsdl.org (SDL2_DIR apuntando a su carpeta cmake).
# z80ex se compila desde ../deps/z80ex.
cmake_minimum_required(VERSION 3.16)
project(alestelx C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(SDL2 REQUIRED)

set(Z80EX ${CMAKE_CURRENT_SOURCE_DIR}/../deps/z80ex)
add_library(z80ex STATIC ${Z80EX}/z80ex.c ${Z80EX}/z80ex_dasm.c)
target_include_directories(z80ex PUBLIC ${Z80EX}/include ${Z80EX})
target_compile_definitions(z80ex PRIVATE WORDS_LITTLE_ENDIAN Z80EX_VERSION_STR=1.1.21 Z80EX_API_REVISION=1
                           Z80EX_VERSION_MAJOR=1 Z80EX_VERSION_MINOR=21 Z80EX_RELEASE_TYPE="")   # "": MSVC no admite el argumento vacio

# LX-GPU (hww): el modelo C++ del submodulo deps/lx-gpu-rtl (emul/) y el
# puente lx/gpu_mmio. El cargador de microcodigo (ROM 0-31) se toma de
# boot/bootloader.hex al configurar y se compila dentro.
set(LXGPU ${CMAKE_CURRENT_SOURCE_DIR}/../deps/lx-gpu-rtl)
if(NOT EXISTS ${LXGPU}/emul/gpu_top.cpp)
  message(FATAL_ERROR "Falta el submodulo deps/lx-gpu-rtl: git submodule update --init")
endif()
file(STRINGS ${LXGPU}/boot/bootloader.hex GPU_BOOT_WORDS LIMIT_COUNT 32)
set(GPU_BOOT_INIT "")
foreach(w IN LISTS GPU_BOOT_WORDS)
  string(STRIP "${w}" w)
  string(APPEND GPU_BOOT_INIT "0x${w}u, ")
endforeach()
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/gpu_boot_rom.h
  "// Generado por CMake desde deps/lx-gpu-rtl/boot/bootloader.hex (ROM 0-31)\n#pragma once\n#include <cstdint>\nstatic const uint32_t GPU_BOOT_ROM[32] = { ${GPU_BOOT_INIT}};\n")
add_library(lxgpu STATIC ${LXGPU}/emul/memory.cpp ${LXGPU}/emul/ch_addr_gen.cpp ${LXGPU}/emul/ch_blender.cpp
            ${LXGPU}/emul/ch_cache.cpp ${LXGPU}/emul/core_alu.cpp ${LXGPU}/emul/gpu_top.cpp ${LXGPU}/emul/host_if.cpp
            lx/gpu_mmio.cpp)
target_include_directories(lxgpu PUBLIC ${LXGPU}/emul ${CMAKE_CURRENT_SOURCE_DIR}/lx ${CMAKE_CURRENT_BINARY_DIR})

add_executable(alestelx lx/main_lx.cpp lx/machine.cpp lx/legacy.cpp lx/fdc765.cpp lx/tape.cpp lx/debugger.cpp lx/gdbserver.cpp)
find_package(Threads REQUIRED)
target_link_libraries(alestelx PRIVATE Threads::Threads)
if(WIN32)
  target_link_libraries(alestelx PRIVATE comdlg32 ws2_32)    # dialogo de ficheros, sockets
endif()
target_link_libraries(alestelx PRIVATE z80ex lxgpu)
if(TARGET SDL2::SDL2)
  if(TARGET SDL2::SDL2main)
    target_link_libraries(alestelx PRIVATE SDL2::SDL2main)    # antes que SDL2 (mingw)
  endif()
  target_link_libraries(alestelx PRIVATE SDL2::SDL2)
else()
  target_include_directories(alestelx PRIVATE ${SDL2_INCLUDE_DIRS})
  target_link_libraries(alestelx PRIVATE ${SDL2_LIBRARIES})
endif()

# Programa de consola tambien en Windows: el depurador lee la terminal
# (arreglo de Valery / hww para Visual Studio + vcpkg)
set_target_properties(alestelx PROPERTIES WIN32_EXECUTABLE FALSE)

# Pruebas: ctest --test-dir build
enable_testing()
add_executable(test_matrix tests/test_matrix.cpp lx/machine.cpp lx/legacy.cpp lx/fdc765.cpp lx/tape.cpp)
target_link_libraries(test_matrix PRIVATE z80ex lxgpu)
add_test(NAME keyboard_matrix COMMAND test_matrix)
