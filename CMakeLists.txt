cmake_minimum_required(VERSION 3.16)
project(project3)

set(CMAKE_C_STANDARD 99)

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)

add_subdirectory(libtdmm)

add_executable(project3 main.c)
target_link_libraries(project3 tdmm)

enable_testing()

add_executable(hello_test testing/hello_test.cc)

target_link_libraries(
  hello_test
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(hello_test)