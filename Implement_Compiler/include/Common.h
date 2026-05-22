#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>
#include <iostream>
#include <sstream>

// ============================================================
//  TOKEN TYPES
// ============================================================
enum class TokenType {
    NUMBER, VARIABLE, STRING_LIT,
    PLUS, MINUS, STAR, SLASH, PERCENT,
    ASSIGN,
    EQ, NEQ, LT, GT, LTE, GTE,
    AND, OR, NOT,
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
    SEMICOLON, COMMA, DOT,
    IF, ELSE, WHILE, DO, FOR, RETURN, BREAK, CONTINUE,
    INT_KW, FLOAT_KW, VOID_KW,
    SWITCH, CASE, DEFAULT,
    END_OF_FILE
};

struct Token {
    TokenType type;
    std::string value;
    int line = 0;

    Token(TokenType t, std::string v, int ln = 0) : type(t), value(v), line(ln) {}
};

// ============================================================
//  AST NODE KINDS
// ============================================================
enum class NodeKind {
    // Expressions
    NUM_LIT, VAR_REF, BINOP, UNOP, ASSIGN_EXPR, CALL,
    // Statements
    EXPR_STMT, IF_STMT, WHILE_STMT, DO_WHILE_STMT,
    FOR_STMT, RETURN_STMT, BREAK_STMT, CONTINUE_STMT,
    BLOCK,
    // Declarations
    VAR_DECL, FUNC_DECL, PROGRAM,
    // Switch
    SWITCH_STMT, CASE_CLAUSE, DEFAULT_CLAUSE
};

struct ASTNode {
    NodeKind kind;
    std::string text;       // operator / identifier / literal text
    double numVal = 0;

    std::vector<std::unique_ptr<ASTNode>> children;

    ASTNode(NodeKind k, std::string t = "") : kind(k), text(t) {}
    ASTNode(double v) : kind(NodeKind::NUM_LIT), numVal(v) {}
};

// ============================================================
//  INSTRUCTION SET (virtual RISC-V-inspired ISA)
// ============================================================
enum class OpCode : uint32_t {
    MOV_CONST = 0, LOAD_VAR, STORE_VAR,
    ADD, SUB, MUL, DIV, MOD,
    CMP,
    BR_EQ, BR_NEQ, BR_LT, BR_GT, BR_LTE, BR_GTE,
    JMP,
    CALL, RET,
    PUSH, POP,
    LOAD_MEM, STORE_MEM,
    PUSH_BP, SET_BP_SP, ALLOC_STACK, RESTORE_BP,
    LOAD_LOCAL, STORE_LOCAL,
    JMP_TABLE,
    HEAP_ALLOC,
    HALT
};

// 4-byte instruction
struct Instruction {
    uint32_t op : 5;
    uint32_t rd : 9;   // destination register
    uint32_t rs1 : 9;   // source 1
    uint32_t rs2 : 9;   // source 2
};
static_assert(sizeof(Instruction) == 4, "Instruction must be 4 bytes");

extern const char* opCodeNames[];

// ============================================================
//  EXEC FILE
// ============================================================
struct SectionHeader {
    uint32_t type;    // 0=Code, 1=Data, 2=SymbolTable
    uint32_t size;
    uint32_t offset;
};

struct ExecFileHeader {
    char     signature[4] = { 'E','X','E','C' };  // 4 bytes
    uint32_t headerSize = sizeof(ExecFileHeader);
    uint32_t sectionCount = 3;
    SectionHeader sections[3];  // Code, Data, SymbolTable
};

struct SymbolEntry {
    char     name[32];
    uint32_t address;
    uint8_t  type;   // 0=func 1=var
};

struct RelocEntry {
    uint32_t instrOffset;
    uint32_t symbolIndex;
};

struct JumpTableEntry {
    int32_t  minVal;
    uint32_t tableId;
    uint32_t targetCount;
};

// RISC-V RV32I-style opcode identifiers (virtual teaching ISA)
namespace RiscV {
    constexpr uint32_t OP_LOAD  = 0x03;
    constexpr uint32_t OP_OP    = 0x33;
    constexpr uint32_t OP_BRANCH = 0x63;
    constexpr uint32_t OP_JAL   = 0x6F;
}