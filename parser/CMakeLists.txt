# parser/CMakeLists.txt
add_library(parser
    src/lexer.cpp
    src/parser.cpp
)

target_include_directories(parser
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
