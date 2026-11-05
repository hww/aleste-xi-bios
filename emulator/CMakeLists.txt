# alestelx: emulador del Aleste LX (modo nativo) para Windows y Linux
#   cmake -S . -B build && cmake --build build
# Necesita SDL2. En Windows: SDL2 de vcpkg o los "development libraries"
# de libsdl.org (SDL2_DIR apuntando a su carpeta cmake).
# z80ex se compila desde ../deps/z80ex. El emulador original (z80_emu, con su
# Makefile) sigue aparte.
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
                           Z80EX_VERSION_MAJOR=1 Z80EX_VERSION_MINOR=21 Z80EX_RELEASE_TYPE=)

add_executable(alestelx lx/main_lx.cpp lx/machine.cpp lx/legacy.cpp lx/fdc765.cpp lx/tape.cpp lx/debugger.cpp)
find_package(Threads REQUIRED)
target_link_libraries(alestelx PRIVATE Threads::Threads)
if(WIN32)
  target_link_libraries(alestelx PRIVATE comdlg32)    # dialogo de ficheros
endif()
target_link_libraries(alestelx PRIVATE z80ex)
if(TARGET SDL2::SDL2)
  if(TARGET SDL2::SDL2main)
    target_link_libraries(alestelx PRIVATE SDL2::SDL2main)    # antes que SDL2 (mingw)
  endif()
  target_link_libraries(alestelx PRIVATE SDL2::SDL2)
else()
  target_include_directories(alestelx PRIVATE ${SDL2_INCLUDE_DIRS})
  target_link_libraries(alestelx PRIVATE ${SDL2_LIBRARIES})
endif()

# Pruebas: ctest --test-dir build
enable_testing()
add_executable(test_matrix tests/test_matrix.cpp lx/machine.cpp lx/legacy.cpp lx/fdc765.cpp lx/tape.cpp)
target_link_libraries(test_matrix PRIVATE z80ex)
add_test(NAME keyboard_matrix COMMAND test_matrix)
