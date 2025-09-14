#pragma once
#include "parser/ast.hpp"
#include <stdexcept>

struct ParseError : public std::runtime_error {
    ParseError(std::string msg) : std::runtime_error(std::move(msg)) {}
};

Statement parse(const std::string &input);
