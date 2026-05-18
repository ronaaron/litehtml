# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/lxk/Desktop/lite-html/litehtml/test_build/litehtml-tests-src"
  "/home/lxk/Desktop/lite-html/litehtml/test_build/litehtml-tests-build"
  "/home/lxk/Desktop/lite-html/litehtml/test_build/litehtml-tests-prefix"
  "/home/lxk/Desktop/lite-html/litehtml/test_build/litehtml-tests-prefix/tmp"
  "/home/lxk/Desktop/lite-html/litehtml/test_build/litehtml-tests-prefix/src/litehtml-tests-stamp"
  "/home/lxk/Desktop/lite-html/litehtml/test_build/litehtml-tests-prefix/src"
  "/home/lxk/Desktop/lite-html/litehtml/test_build/litehtml-tests-prefix/src/litehtml-tests-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/lxk/Desktop/lite-html/litehtml/test_build/litehtml-tests-prefix/src/litehtml-tests-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/lxk/Desktop/lite-html/litehtml/test_build/litehtml-tests-prefix/src/litehtml-tests-stamp${cfgdir}") # cfgdir has leading slash
endif()
