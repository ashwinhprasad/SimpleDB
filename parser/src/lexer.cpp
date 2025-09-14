#include "lexer.hpp"
#include <cctype>

static bool is_ident_start(char c) { return std::isalpha(c) || c == '_'; }
static bool is_ident_continue(char c) { return std::isalnum(c) || c == '_'; }

std::vector<TokenValue> lex(const std::string &input) {
    std::vector<TokenValue> tokens;
    size_t i = 0;

    while (i < input.size()) {
        char c = input[i];
        switch (c) {
            case ' ': case '\t': case '\n': case '\r': i++; break;
            case '(': tokens.push_back({Token::LParen}); i++; break;
            case ')': tokens.push_back({Token::RParen}); i++; break;
            case ',': tokens.push_back({Token::Comma}); i++; break;
            case ';': tokens.push_back({Token::Semi}); i++; break;
            case '*': tokens.push_back({Token::Star}); i++; break;
            case '.': tokens.push_back({Token::Dot}); i++; break;
            case '=': tokens.push_back({Token::Eq}); i++; break;
            case '!':
                i++;
                if (i < input.size() && input[i] == '=') { tokens.push_back({Token::Neq}); i++; }
                else throw LexError("Unexpected '!'", i);
                break;
            case '<':
                i++;
                if (i < input.size() && input[i] == '=') { tokens.push_back({Token::Lte}); i++; }
                else tokens.push_back({Token::Lt});
                break;
            case '>':
                i++;
                if (i < input.size() && input[i] == '=') { tokens.push_back({Token::Gte}); i++; }
                else tokens.push_back({Token::Gt});
                break;
            case '\'' : {
                i++;
                std::string s;
                while (i < input.size()) {
                    if (input[i] == '\'') {
                        if (i+1 < input.size() && input[i+1] == '\'') { s.push_back('\''); i+=2; }
                        else { i++; break; }
                    } else {
                        s.push_back(input[i++]);
                    }
                }
                tokens.push_back({Token::String, s});
                break;
            }
            default:
                if (std::isdigit(c)) {
                    std::string num;
                    bool is_float = false;
                    while (i < input.size() && std::isdigit(input[i])) num.push_back(input[i++]);
                    if (i < input.size() && input[i] == '.') {
                        is_float = true;
                        num.push_back('.');
                        i++;
                        while (i < input.size() && std::isdigit(input[i])) num.push_back(input[i++]);
                    }
                    if (is_float) {
                        tokens.push_back({Token::Float, num, 0, std::stod(num)});
                    } else {
                        tokens.push_back({Token::Int, num, std::stoll(num)});
                    }
                } else if (is_ident_start(c)) {
                    std::string s;
                    while (i < input.size() && is_ident_continue(input[i])) s.push_back(input[i++]);
                    auto kw = to_keyword(s);
                    if (kw) tokens.push_back({*kw, s});
                    else tokens.push_back({Token::Ident, s});
                } else {
                    throw LexError("Unexpected char", i);
                }
        }
    }
    return tokens;
}
