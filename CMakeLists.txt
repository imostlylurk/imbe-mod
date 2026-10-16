cmake_minimum_required(VERSION 3.13)
project(imbe-mod C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

file(GLOB IMBE_SRC src/*.cc)
list(FILTER IMBE_SRC EXCLUDE REGEX ".*/wrapper\\.cc$")

add_library(imbe STATIC ${IMBE_SRC} src/wrapper.cc)
target_include_directories(imbe PUBLIC inc)
# integer-only code: flags do not change numerics; -O3 is safe
target_compile_options(imbe PRIVATE -O3 -Wall -Wextra -flto=auto)
target_link_options(imbe INTERFACE -flto=auto)

add_executable(imbebench tests/imbebench.c)
target_link_libraries(imbebench PRIVATE imbe m)
target_compile_options(imbebench PRIVATE -O2)

enable_testing()
add_test(NAME imbebench COMMAND imbebench --selftest)
