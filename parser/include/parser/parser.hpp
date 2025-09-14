#pragma once
#include "ast.hpp"
#include "lexer.hpp"

struct ParseError : public std::runtime_error {
    ParseError(std::string msg) : std::runtime_error(std::move(msg)) {}
};

Statement parse(const std::string &input);
