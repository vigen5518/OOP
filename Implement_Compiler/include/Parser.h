#pragma once
#include "Common.h"
#include <stdexcept>

class Parser {
public:
    explicit Parser(std::vector<Token> toks) : tokens(std::move(toks)), pos(0) {}

    // Parse entire program: list of function/variable declarations
    std::unique_ptr<ASTNode> parseProgram() {
        auto prog = std::make_unique<ASTNode>(NodeKind::PROGRAM);
        while (!check(TokenType::END_OF_FILE)) {
            prog->children.push_back(parseDeclaration());
        }
        return prog;
    }

private:
    std::vector<Token> tokens;
    size_t pos;

    Token& cur() { return tokens[pos]; }
    bool check(TokenType t) { return cur().type == t; }
    bool isType() { return check(TokenType::INT_KW) || check(TokenType::FLOAT_KW) || check(TokenType::VOID_KW); }

    Token consume(TokenType t, const std::string& msg) {
        if (!check(t)) throw std::runtime_error("Parse error at line " + std::to_string(cur().line) + ": " + msg + " (got '" + cur().value + "')");
        return tokens[pos++];
    }

    Token advance() { return tokens[pos++]; }

    // -------------------------------------------------------
    //  DECLARATIONS
    // -------------------------------------------------------
    std::unique_ptr<ASTNode> parseDeclaration() {
        // type ident ...
        if (isType()) {
            Token typeKw = advance();
            Token name = consume(TokenType::VARIABLE, "expected identifier");
            if (check(TokenType::LPAREN)) {
                // Function declaration
                return parseFuncDecl(typeKw.value, name.value);
            }
            else {
                // Variable declaration
                return parseVarDecl(typeKw.value, name.value);
            }
        }
        return parseStatement();
    }

    std::unique_ptr<ASTNode> parseFuncDecl(const std::string& retType, const std::string& name) {
        auto node = std::make_unique<ASTNode>(NodeKind::FUNC_DECL, name);
        node->text = retType + " " + name;
        consume(TokenType::LPAREN, "expected '('");
        // Parameters
        while (!check(TokenType::RPAREN) && !check(TokenType::END_OF_FILE)) {
            if (isType()) advance();
            auto param = std::make_unique<ASTNode>(NodeKind::VAR_DECL, cur().value);
            advance();
            node->children.push_back(std::move(param));
            if (check(TokenType::COMMA)) advance();
        }
        consume(TokenType::RPAREN, "expected ')'");
        node->children.push_back(parseBlock());
        return node;
    }

    std::unique_ptr<ASTNode> parseVarDecl(const std::string& type, const std::string& name) {
        auto node = std::make_unique<ASTNode>(NodeKind::VAR_DECL, name);
        node->text = type + " " + name;
        if (check(TokenType::ASSIGN)) {
            advance();
            node->children.push_back(parseExpression());
        }
        consume(TokenType::SEMICOLON, "expected ';'");
        return node;
    }

    // -------------------------------------------------------
    //  STATEMENTS
    // -------------------------------------------------------
    std::unique_ptr<ASTNode> parseStatement() {
        if (check(TokenType::LBRACE))  return parseBlock();
        if (check(TokenType::IF))      return parseIf();
        if (check(TokenType::WHILE))   return parseWhile();
        if (check(TokenType::DO))      return parseDoWhile();
        if (check(TokenType::FOR))     return parseFor();
        if (check(TokenType::SWITCH))  return parseSwitch();
        if (check(TokenType::RETURN))  return parseReturn();
        if (check(TokenType::BREAK)) {
            advance();
            consume(TokenType::SEMICOLON, "expected ';'");
            return std::make_unique<ASTNode>(NodeKind::BREAK_STMT);
        }
        if (check(TokenType::CONTINUE)) {
            advance();
            consume(TokenType::SEMICOLON, "expected ';'");
            return std::make_unique<ASTNode>(NodeKind::CONTINUE_STMT);
        }
        if (isType()) {
            Token typeKw = advance();
            Token name = consume(TokenType::VARIABLE, "expected identifier");
            return parseVarDecl(typeKw.value, name.value);
        }
        // Expression statement
        auto expr = parseExpression();
        consume(TokenType::SEMICOLON, "expected ';'");
        auto stmt = std::make_unique<ASTNode>(NodeKind::EXPR_STMT);
        stmt->children.push_back(std::move(expr));
        return stmt;
    }

    std::unique_ptr<ASTNode> parseBlock() {
        consume(TokenType::LBRACE, "expected '{'");
        auto block = std::make_unique<ASTNode>(NodeKind::BLOCK);
        while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE))
            block->children.push_back(parseDeclaration());
        consume(TokenType::RBRACE, "expected '}'");
        return block;
    }

    std::unique_ptr<ASTNode> parseIf() {
        advance(); // if
        auto node = std::make_unique<ASTNode>(NodeKind::IF_STMT);
        consume(TokenType::LPAREN, "expected '('");
        node->children.push_back(parseExpression()); // condition
        consume(TokenType::RPAREN, "expected ')'");
        node->children.push_back(parseStatement());  // then
        if (check(TokenType::ELSE)) {
            advance();
            node->children.push_back(parseStatement()); // else
        }
        return node;
    }

    std::unique_ptr<ASTNode> parseWhile() {
        advance(); // while
        auto node = std::make_unique<ASTNode>(NodeKind::WHILE_STMT);
        consume(TokenType::LPAREN, "expected '('");
        node->children.push_back(parseExpression());
        consume(TokenType::RPAREN, "expected ')'");
        node->children.push_back(parseStatement());
        return node;
    }

    std::unique_ptr<ASTNode> parseDoWhile() {
        advance(); // do
        auto node = std::make_unique<ASTNode>(NodeKind::DO_WHILE_STMT);
        node->children.push_back(parseStatement());  // body
        consume(TokenType::WHILE, "expected 'while'");
        consume(TokenType::LPAREN, "expected '('");
        node->children.push_back(parseExpression()); // condition
        consume(TokenType::RPAREN, "expected ')'");
        consume(TokenType::SEMICOLON, "expected ';'");
        return node;
    }

    std::unique_ptr<ASTNode> parseFor() {
        advance(); // for
        auto node = std::make_unique<ASTNode>(NodeKind::FOR_STMT);
        consume(TokenType::LPAREN, "expected '('");
        // init
        if (!check(TokenType::SEMICOLON)) {
            if (isType()) {
                Token tk = advance();
                Token nm = consume(TokenType::VARIABLE, "expected name");
                node->children.push_back(parseVarDecl(tk.value, nm.value));
            }
            else {
                node->children.push_back(parseExpression());
                consume(TokenType::SEMICOLON, "expected ';'");
            }
        }
        else {
            advance();
            node->children.push_back(nullptr);
        }
        // cond
        if (!check(TokenType::SEMICOLON))
            node->children.push_back(parseExpression());
        else
            node->children.push_back(nullptr);
        consume(TokenType::SEMICOLON, "expected ';'");
        // step
        if (!check(TokenType::RPAREN))
            node->children.push_back(parseExpression());
        else
            node->children.push_back(nullptr);
        consume(TokenType::RPAREN, "expected ')'");
        node->children.push_back(parseStatement()); // body
        return node;
    }

    std::unique_ptr<ASTNode> parseSwitch() {
        advance(); // switch
        auto node = std::make_unique<ASTNode>(NodeKind::SWITCH_STMT);
        consume(TokenType::LPAREN, "expected '('");
        node->children.push_back(parseExpression());
        consume(TokenType::RPAREN, "expected ')'");
        consume(TokenType::LBRACE, "expected '{'");
        while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
            if (check(TokenType::CASE)) {
                advance();
                auto cc = std::make_unique<ASTNode>(NodeKind::CASE_CLAUSE);
                cc->children.push_back(parseExpression());
                consume(TokenType::SEMICOLON, "expected ':'"); // using ; as case separator
                while (!check(TokenType::CASE) && !check(TokenType::DEFAULT) &&
                    !check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE))
                    cc->children.push_back(parseStatement());
                node->children.push_back(std::move(cc));
            }
            else if (check(TokenType::DEFAULT)) {
                advance();
                consume(TokenType::SEMICOLON, "expected ':'");
                auto dc = std::make_unique<ASTNode>(NodeKind::DEFAULT_CLAUSE);
                while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE))
                    dc->children.push_back(parseStatement());
                node->children.push_back(std::move(dc));
            }
            else {
                node->children.push_back(parseStatement());
            }
        }
        consume(TokenType::RBRACE, "expected '}'");
        return node;
    }

    std::unique_ptr<ASTNode> parseReturn() {
        advance(); // return
        auto node = std::make_unique<ASTNode>(NodeKind::RETURN_STMT);
        if (!check(TokenType::SEMICOLON))
            node->children.push_back(parseExpression());
        consume(TokenType::SEMICOLON, "expected ';'");
        return node;
    }

    // -------------------------------------------------------
    //  EXPRESSIONS  (precedence climbing)
    // -------------------------------------------------------
    std::unique_ptr<ASTNode> parseExpression() {
        return parseAssign();
    }

    std::unique_ptr<ASTNode> parseAssign() {
        auto left = parseLogicOr();
        if (check(TokenType::ASSIGN)) {
            advance();
            auto node = std::make_unique<ASTNode>(NodeKind::ASSIGN_EXPR, "=");
            node->children.push_back(std::move(left));
            node->children.push_back(parseAssign());
            return node;
        }
        return left;
    }

    std::unique_ptr<ASTNode> parseLogicOr() {
        auto left = parseLogicAnd();
        while (check(TokenType::OR)) {
            Token op = advance();
            auto node = std::make_unique<ASTNode>(NodeKind::BINOP, op.value);
            node->children.push_back(std::move(left));
            node->children.push_back(parseLogicAnd());
            left = std::move(node);
        }
        return left;
    }

    std::unique_ptr<ASTNode> parseLogicAnd() {
        auto left = parseEquality();
        while (check(TokenType::AND)) {
            Token op = advance();
            auto node = std::make_unique<ASTNode>(NodeKind::BINOP, op.value);
            node->children.push_back(std::move(left));
            node->children.push_back(parseEquality());
            left = std::move(node);
        }
        return left;
    }

    std::unique_ptr<ASTNode> parseEquality() {
        auto left = parseRelational();
        while (check(TokenType::EQ) || check(TokenType::NEQ)) {
            Token op = advance();
            auto node = std::make_unique<ASTNode>(NodeKind::BINOP, op.value);
            node->children.push_back(std::move(left));
            node->children.push_back(parseRelational());
            left = std::move(node);
        }
        return left;
    }

    std::unique_ptr<ASTNode> parseRelational() {
        auto left = parseAddSub();
        while (check(TokenType::LT) || check(TokenType::GT) || check(TokenType::LTE) || check(TokenType::GTE)) {
            Token op = advance();
            auto node = std::make_unique<ASTNode>(NodeKind::BINOP, op.value);
            node->children.push_back(std::move(left));
            node->children.push_back(parseAddSub());
            left = std::move(node);
        }
        return left;
    }

    std::unique_ptr<ASTNode> parseAddSub() {
        auto left = parseMulDiv();
        while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
            Token op = advance();
            auto node = std::make_unique<ASTNode>(NodeKind::BINOP, op.value);
            node->children.push_back(std::move(left));
            node->children.push_back(parseMulDiv());
            left = std::move(node);
        }
        return left;
    }

    std::unique_ptr<ASTNode> parseMulDiv() {
        auto left = parseUnary();
        while (check(TokenType::STAR) || check(TokenType::SLASH) || check(TokenType::PERCENT)) {
            Token op = advance();
            auto node = std::make_unique<ASTNode>(NodeKind::BINOP, op.value);
            node->children.push_back(std::move(left));
            node->children.push_back(parseUnary());
            left = std::move(node);
        }
        return left;
    }

    std::unique_ptr<ASTNode> parseUnary() {
        if (check(TokenType::MINUS) || check(TokenType::NOT)) {
            Token op = advance();
            auto node = std::make_unique<ASTNode>(NodeKind::UNOP, op.value);
            node->children.push_back(parseUnary());
            return node;
        }
        return parsePrimary();
    }

    std::unique_ptr<ASTNode> parsePrimary() {
        if (check(TokenType::NUMBER)) {
            Token t = advance();
            return std::make_unique<ASTNode>(std::stod(t.value));
        }
        if (check(TokenType::VARIABLE)) {
            Token t = advance();
            // Function call?
            if (check(TokenType::LPAREN)) {
                advance();
                auto call = std::make_unique<ASTNode>(NodeKind::CALL, t.value);
                while (!check(TokenType::RPAREN) && !check(TokenType::END_OF_FILE)) {
                    call->children.push_back(parseExpression());
                    if (check(TokenType::COMMA)) advance();
                }
                consume(TokenType::RPAREN, "expected ')'");
                return call;
            }
            return std::make_unique<ASTNode>(NodeKind::VAR_REF, t.value);
        }
        if (check(TokenType::LPAREN)) {
            advance();
            auto inner = parseExpression();
            consume(TokenType::RPAREN, "expected ')'");
            return inner;
        }
        throw std::runtime_error("Unexpected token '" + cur().value + "' at line " + std::to_string(cur().line));
    }
};