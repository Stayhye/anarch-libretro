cmake_minimum_required(VERSION 3.7)
project(anarch-libretro C)


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
elseif (LIBRETRO_STATIC)
  set_target_properties(anarch_libretro PROPERTIES SUFFIX "${LIBRETRO_SUFFIX}.a")
endif ()
