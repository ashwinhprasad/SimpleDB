#pragma once
#include <string>
#include <optional>
#include <unordered_map>

enum class Token {
    // punctuation
    LParen, RParen, Comma, Plus,
    Semi, Star, Eq, Neq, Lt, Lte, Gt, Gte, Dot,

    // literals
    Ident, String, Int, Float,

    // keywords
    KwCreate, KwTable, KwDrop, KwIf, KwExists,
    KwInsert, KwInto, KwValues, KwDelete, KwFrom,
    KwWhere, KwUpdate, KwSet, KwSelect,
    KwAnd, KwOr, KwNot,
    KwNull, KwTrue, KwFalse, KwLimit,

    // simple types
    KwInt, KwInteger, KwText, KwReal, KwFloat, KwBool,
};

struct TokenValue {
    Token kind;
    std::string text;   // for Ident, String
    long long int_val{}; // for Int
    double float_val{};  // for Float
};

inline std::optional<Token> to_keyword(const std::string &s) {
    static const std::unordered_map<std::string, Token> kwmap = {
        {"CREATE", Token::KwCreate}, {"TABLE", Token::KwTable},
        {"DROP", Token::KwDrop}, {"IF", Token::KwIf}, {"EXISTS", Token::KwExists},
        {"INSERT", Token::KwInsert}, {"INTO", Token::KwInto}, {"VALUES", Token::KwValues},
        {"DELETE", Token::KwDelete}, {"FROM", Token::KwFrom}, {"WHERE", Token::KwWhere},
        {"UPDATE", Token::KwUpdate}, {"SET", Token::KwSet}, {"SELECT", Token::KwSelect},
        {"AND", Token::KwAnd}, {"OR", Token::KwOr}, {"NOT", Token::KwNot},
        {"NULL", Token::KwNull}, {"TRUE", Token::KwTrue}, {"FALSE", Token::KwFalse},
        {"LIMIT", Token::KwLimit},
        {"INT", Token::KwInt}, {"INTEGER", Token::KwInteger}, {"TEXT", Token::KwText},
        {"REAL", Token::KwReal}, {"FLOAT", Token::KwFloat}, {"BOOL", Token::KwBool}
    };

    std::string up;
    up.reserve(s.size());
    for (char c : s) up.push_back(::toupper(c));

    auto it = kwmap.find(up);
    if (it != kwmap.end()) return it->second;
    return std::nullopt;
}
