#pragma once
#include "Common.h"
#include <cctype>
#include <stdexcept>

class Lexer {
public:
    explicit Lexer(const std::string& src) : src(src), pos(0), line(1) {}

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        while (pos < src.size()) {
            skipWhitespace();
            if (pos >= src.size()) break;

            // Comments
            if (pos + 1 < src.size() && src[pos] == '/' && src[pos + 1] == '/') {
                while (pos < src.size() && src[pos] != '\n') pos++;
                continue;
            }

            char c = src[pos];

            // Number literal
            if (isdigit(c) || (c == '.' && pos + 1 < src.size() && isdigit(src[pos + 1]))) {
                tokens.push_back(readNumber());
                continue;
            }

            // String literal
            if (c == '"') {
                tokens.push_back(readString());
                continue;
            }

            // Identifier or keyword
            if (isalpha(c) || c == '_') {
                tokens.push_back(readIdent());
                continue;
            }

            // Operators & punctuation
            tokens.push_back(readSymbol());
        }
        tokens.push_back({ TokenType::END_OF_FILE, "", line });
        return tokens;
    }

private:
    std::string src;
    size_t pos;
    int line;

    void skipWhitespace() {
        while (pos < src.size() && (src[pos] == ' ' || src[pos] == '\t' || src[pos] == '\r' || src[pos] == '\n')) {
            if (src[pos] == '\n') line++;
            pos++;
        }
    }

    Token readNumber() {
        std::string t;
        bool hasDot = false;
        while (pos < src.size() && (isdigit(src[pos]) || (src[pos] == '.' && !hasDot))) {
            if (src[pos] == '.') hasDot = true;
            t += src[pos++];
        }
        return { TokenType::NUMBER, t, line };
    }

    Token readString() {
        pos++; // skip "
        std::string t;
        while (pos < src.size() && src[pos] != '"') {
            if (src[pos] == '\\' && pos + 1 < src.size()) { pos++; t += src[pos]; }
            else t += src[pos];
            pos++;
        }
        pos++; // skip "
        return { TokenType::STRING_LIT, t, line };
    }

    Token readIdent() {
        std::string t;
        while (pos < src.size() && (isalnum(src[pos]) || src[pos] == '_')) t += src[pos++];
        // Keywords
        if (t == "if")       return { TokenType::IF,       t, line };
        if (t == "else")     return { TokenType::ELSE,     t, line };
        if (t == "while")    return { TokenType::WHILE,    t, line };
        if (t == "do")       return { TokenType::DO,       t, line };
        if (t == "for")      return { TokenType::FOR,      t, line };
        if (t == "return")   return { TokenType::RETURN,   t, line };
        if (t == "break")    return { TokenType::BREAK,    t, line };
        if (t == "continue") return { TokenType::CONTINUE, t, line };
        if (t == "int")      return { TokenType::INT_KW,   t, line };
        if (t == "float")    return { TokenType::FLOAT_KW, t, line };
        if (t == "void")     return { TokenType::VOID_KW,  t, line };
        if (t == "switch")   return { TokenType::SWITCH,   t, line };
        if (t == "case")     return { TokenType::CASE,     t, line };
        if (t == "default")  return { TokenType::DEFAULT,  t, line };
        return { TokenType::VARIABLE, t, line };
    }

    Token readSymbol() {
        char c = src[pos++];
        auto peek = [&]() { return pos < src.size() ? src[pos] : '\0'; };
        switch (c) {
        case '+': return { TokenType::PLUS,      "+",  line };
        case '-': return { TokenType::MINUS,     "-",  line };
        case '*': return { TokenType::STAR,      "*",  line };
        case '/': return { TokenType::SLASH,     "/",  line };
        case '%': return { TokenType::PERCENT,   "%",  line };
        case '(': return { TokenType::LPAREN,    "(",  line };
        case ')': return { TokenType::RPAREN,    ")",  line };
        case '{': return { TokenType::LBRACE,    "{",  line };
        case '}': return { TokenType::RBRACE,    "}",  line };
        case '[': return { TokenType::LBRACKET,  "[",  line };
        case ']': return { TokenType::RBRACKET,  "]",  line };
        case ';': return { TokenType::SEMICOLON, ";",  line };
        case ',': return { TokenType::COMMA,     ",",  line };
        case '.': return { TokenType::DOT,       ".",  line };
        case '=':
            if (peek() == '=') { pos++; return { TokenType::EQ,  "==", line }; }
            return { TokenType::ASSIGN, "=", line };
        case '!':
            if (peek() == '=') { pos++; return { TokenType::NEQ, "!=", line }; }
            return { TokenType::NOT, "!", line };
        case '<':
            if (peek() == '=') { pos++; return { TokenType::LTE, "<=", line }; }
            return { TokenType::LT,  "<",  line };
        case '>':
            if (peek() == '=') { pos++; return { TokenType::GTE, ">=", line }; }
            return { TokenType::GT,  ">",  line };
        case '&':
            if (peek() == '&') { pos++; return { TokenType::AND, "&&", line }; }
            break;
        case '|':
            if (peek() == '|') { pos++; return { TokenType::OR, "||", line }; }
            break;
        default: break;
        }
        return { TokenType::END_OF_FILE, std::string(1,c), line };
    }
};