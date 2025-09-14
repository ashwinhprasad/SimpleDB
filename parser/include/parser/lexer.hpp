#pragma once
#include "token.hpp"
#include <string>
#include <vector>
#include <stdexcept>

struct LexError : public std::runtime_error {
    size_t pos;
    LexError(std::string msg, size_t p) : std::runtime_error(std::move(msg)), pos(p) {}
};

std::vector<TokenValue> lex(const std::string &input);
