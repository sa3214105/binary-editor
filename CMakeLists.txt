# 指定 CMake 的最低版本
cmake_minimum_required(VERSION 3.10)

# 設置 vcpkg 工具鏈文件
# set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"  CACHE STRING "")
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"  CACHE STRING "")

# 設置項目名稱
project(cpp_cache)

# 設置 C++ 標準
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 查找 Google Test 包
find_package(GTest REQUIRED)

# 添加包含目錄 (可選，如果有頭文件)
include_directories(./src ${GTEST_INCLUDE_DIRS})

# 添加源文件並生成可執行文件
add_executable(cpp_cache ./unit_test/main.cpp)

# 指定輸出目錄
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 添加單元測試可執行文件
add_executable(unit_binary_editor ./unit_test/unit_binary_editor.cpp)

# 連接 Google Test 庫
target_link_libraries(unit_binary_editor GTest::gtest GTest::gtest_main)

# 添加測試
enable_testing()
add_test(NAME unit_binary_editor_test COMMAND unit_binary_editor)