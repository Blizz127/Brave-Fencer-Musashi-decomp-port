# Cross toolchain: x86-64 Windows from Linux with zig (MinGW ABI, UCRT).
#   cmake -S . -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/zig-windows.cmake \
#         -DMUSASHI_WINDOWS_DEPS=<dir holding sdl2/ and openal/ (docs/SETUP.md)>
# zig must be on PATH or named by ZIG. The native lane's generator runs on the
# build host and emits COFF lane objects (--host-target).
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(_zig_wrap "${CMAKE_BINARY_DIR}/zig-windows")
file(MAKE_DIRECTORY "${_zig_wrap}")
foreach(_tool cc c++ ar ranlib rc)
    file(CREATE_LINK "${CMAKE_CURRENT_LIST_DIR}/../tools/zig-windows-cc.sh"
         "${_zig_wrap}/zig-windows-${_tool}" SYMBOLIC)
endforeach()
set(CMAKE_C_COMPILER "${_zig_wrap}/zig-windows-cc")
set(CMAKE_CXX_COMPILER "${_zig_wrap}/zig-windows-c++")
set(CMAKE_AR "${_zig_wrap}/zig-windows-ar" CACHE FILEPATH "")
set(CMAKE_RANLIB "${_zig_wrap}/zig-windows-ranlib" CACHE FILEPATH "")
set(CMAKE_RC_COMPILER "${_zig_wrap}/zig-windows-rc")

set(MUSASHI_LANE_HOST_TARGET "x86_64-windows-gnu" CACHE STRING "")

# zig cc builds without -O as Debug with UBSan in trap mode (ud1). The Linux
# build's GCC has no sanitizer; match it.
set(CMAKE_C_FLAGS_INIT "-fno-sanitize=undefined")
set(CMAKE_CXX_FLAGS_INIT "-fno-sanitize=undefined")
# zig links with /DEBUG unless stripped, and lld-link then defaults to
# /OPT:NOREF: unreferenced COMDATs stay, among them LIBGTE's rcossin table,
# which equals the game's own (retail_guard). The Linux build drops it with
# --gc-sections; a stripped link gets /OPT:REF.
set(CMAKE_EXE_LINKER_FLAGS_INIT "-s")

# Programs (python, clang for the lane generator) come from the build host;
# libraries and headers only from the Windows dependency prefixes.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
if(MUSASHI_WINDOWS_DEPS)
    set(_sdl "${MUSASHI_WINDOWS_DEPS}/sdl2/SDL2-2.32.10/x86_64-w64-mingw32")
    set(_oal "${MUSASHI_WINDOWS_DEPS}/openal/openal-soft-1.25.1-bin")
    list(APPEND CMAKE_FIND_ROOT_PATH "${_sdl}" "${_oal}")
    set(SDL2_DIR "${_sdl}/lib/cmake/SDL2" CACHE PATH "")
    set(OPENAL_INCLUDE_DIR "${_oal}/include/AL" CACHE PATH "")
    set(OPENAL_LIBRARY "${_oal}/libs/Win64/libOpenAL32.dll.a" CACHE FILEPATH "")
endif()
