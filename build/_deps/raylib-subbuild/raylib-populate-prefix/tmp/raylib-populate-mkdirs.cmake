# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "D:/VScode Projects/Game_Development/build/_deps/raylib-src"
  "D:/VScode Projects/Game_Development/build/_deps/raylib-build"
  "D:/VScode Projects/Game_Development/build/_deps/raylib-subbuild/raylib-populate-prefix"
  "D:/VScode Projects/Game_Development/build/_deps/raylib-subbuild/raylib-populate-prefix/tmp"
  "D:/VScode Projects/Game_Development/build/_deps/raylib-subbuild/raylib-populate-prefix/src/raylib-populate-stamp"
  "D:/VScode Projects/Game_Development/build/_deps/raylib-subbuild/raylib-populate-prefix/src"
  "D:/VScode Projects/Game_Development/build/_deps/raylib-subbuild/raylib-populate-prefix/src/raylib-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/VScode Projects/Game_Development/build/_deps/raylib-subbuild/raylib-populate-prefix/src/raylib-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/VScode Projects/Game_Development/build/_deps/raylib-subbuild/raylib-populate-prefix/src/raylib-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
