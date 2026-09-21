cmake_minimum_required(VERSION 3.7)
project(anarch-libretro C)

# Force static linking for PlayStation 2 or if explicitly requested
if (CMAKE_SYSTEM_NAME STREQUAL "PS2" OR DEFINED ENV{PS2SDK})
    set(LIBRETRO_STATIC ON)
endif()

option(LIBRETRO_STATIC "Statically link the libretro core" OFF)

if (LIBRETRO_STATIC)
  add_library(anarch_libretro STATIC main_libretro.c)
else ()
  add_library(anarch_libretro SHARED main_libretro.c)
endif ()

set_property(TARGET anarch_libretro PROPERTY C_STANDARD 99)

# Follow naming conventions for libretro cores
set_target_properties(anarch_libretro PROPERTIES PREFIX "")
if (ANDROID)
  set_target_properties(anarch_libretro PROPERTIES SUFFIX "_android.so")
elseif (EMSCRIPTEN)
  set_target_properties(anarch_libretro PROPERTIES SUFFIX "${LIBRETRO_SUFFIX}.bc")
elseif (CMAKE_SYSTEM_NAME STREQUAL "PS2" OR DEFINED ENV{PS2SDK} OR LIBRETRO_STATIC)
  set_target_properties(anarch_libretro PROPERTIES SUFFIX "_ps2.a")
endif ()