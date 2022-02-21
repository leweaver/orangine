project(OeApp VERSION 1.0
        DESCRIPTION "Orangine App Library - Tests"
        LANGUAGES CXX)

include(GoogleTest)

add_executable(OeAppTests test_yaml_config_reader.cpp tests_main.cpp tests_main.h)

find_package(GTest REQUIRED)
target_link_libraries(OeAppTests
PRIVATE
    Oe::App GTest::gtest GTest::gmock
)

gtest_discover_tests(OeAppTests)