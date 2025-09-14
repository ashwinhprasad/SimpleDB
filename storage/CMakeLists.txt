# parser/CMakeLists.txt
add_library(storage
    src/disk.cpp
)

target_include_directories(storage
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/storage>
)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif()