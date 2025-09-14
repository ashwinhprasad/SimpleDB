find_package(GTest CONFIG REQUIRED)

add_executable(test_disk test_disk.cpp)

target_link_libraries(test_disk
    PRIVATE
        storage
        GTest::gtest
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_disk)
