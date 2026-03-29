#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <iomanip>
#include <memory>
#include <cstdint>

using namespace std;

enum class OpCode : uint32_t {MOV_CONST, LOAD_VAR, ADD, SUB, MUL, DIV, HALT};
string opNames[]={"MOV_CONST", "LOAD_VAR", "ADD", "SUB", "MUL", "DIV", "HALT"};

struct Instruction {
    uint32_t op       : 4;  
    uint32_t resIdx   : 9;  
    uint32_t leftIdx  : 9;  
    uint32_t rightIdx : 9;  
};

static_assert(sizeof(Instruction)==4, "Instruction structure must be 4 bytes");

enum class NodeKind {NUM_NODE, VAR_NODE, OP_NODE};

struct ExprNode {
    NodeKind kind;
    string text;
    double val;
    unique_ptr<ExprNode> left;
    unique_ptr<ExprNode> right;

    ExprNode(NodeKind k, string t) : kind(k), text(t), val(0) {}
    ExprNode(double v) : kind(NodeKind::NUM_NODE), text(""), val(v) {}
};

enum TokenType {NUMBER, VARIABLE, OPERATOR, LPAREN, RPAREN};
struct Token {
    TokenType type;
    string value;
};

vector<Token> tokenList;
int curIdx=0;

void tokenize(string input) {
    tokenList.clear();
    curIdx=0;
    for (int i=0; i<input.length(); ++i) {
        if (isspace(input[i])) 
            continue;
        if (isdigit(input[i])) {
            string t;
            while (i<input.length() && (isdigit(input[i]) || input[i]=='.')) 
                t+=input[i++];
            tokenList.push_back({NUMBER, t});
            i--;
        } 
        else if (isalpha(input[i])) {
            string t;
            while (i<input.length() && isalnum(input[i])) 
                t+=input[i++];
            tokenList.push_back({VARIABLE, t});
            i--;
        } 
        else {
            TokenType type=(input[i]=='(') ? LPAREN : (input[i]==')') ? RPAREN : OPERATOR;
            tokenList.push_back({type, string(1, input[i])});
        }
    }
}

unique_ptr<ExprNode> parseExpr();

unique_ptr<ExprNode> parsePrimary() {
    if (curIdx>=tokenList.size()) 
        return nullptr;
    Token t=tokenList[curIdx++];
    if (t.type==NUMBER) 
        return make_unique<ExprNode>(stod(t.value));
    if (t.type==VARIABLE) 
        return make_unique<ExprNode>(NodeKind::VAR_NODE, t.value);
    if (t.type==LPAREN) {
        auto n=parseExpr();
        if (curIdx<tokenList.size()) 
            curIdx++; 
        return n;
    }
    return nullptr;
}

unique_ptr<ExprNode> parseTerm() {
    auto l=parsePrimary();
    while (curIdx<tokenList.size() && (tokenList[curIdx].value=="*" || tokenList[curIdx].value=="/")) {
        string op=tokenList[curIdx++].value;
        auto p=make_unique<ExprNode>(NodeKind::OP_NODE, op);
        p->left=std::move(l);
        p->right=parsePrimary();
        l=std::move(p);
    }
    return l;
}

unique_ptr<ExprNode> parseExpr() {
    auto l=parseTerm();
    while (curIdx<tokenList.size() && (tokenList[curIdx].value=="+" || tokenList[curIdx].value=="-")) {
        string op=tokenList[curIdx++].value;
        auto p=make_unique<ExprNode>(NodeKind::OP_NODE, op);
        p->left=std::move(l);
        p->right=parseTerm();
        l=std::move(p);
    }
    return l;
}

int compile(const ExprNode* node, vector<Instruction>& program, vector<double>& constPool, int& nextReg, map<string, int>& varMap) {
    if (!node) 
        return -1;
    if (node->kind==NodeKind::NUM_NODE) {
        int r=nextReg++;
        uint32_t constIdx=constPool.size();
        constPool.push_back(node->val);
        program.push_back({(uint32_t)OpCode::MOV_CONST, (uint32_t)r, constIdx, 0});
        return r;
    }
    if (node->kind==NodeKind::VAR_NODE) {
        int r=nextReg++;
        if (varMap.find(node->text)==varMap.end()) {
            int id=varMap.size();
            varMap[node->text]=id;
        }
        program.push_back({(uint32_t)OpCode::LOAD_VAR, (uint32_t)r, (uint32_t)varMap[node->text], 0});
        return r;
    }
    int l=compile(node->left.get(), program, constPool, nextReg, varMap);
    int r=compile(node->right.get(), program, constPool, nextReg, varMap);
    int target=nextReg++;
    OpCode op=(node->text=="+") ? OpCode::ADD : (node->text=="-") ? OpCode::SUB : (node->text=="*") ? OpCode::MUL : OpCode::DIV;
    program.push_back({(uint32_t)op, (uint32_t)target, (uint32_t)l, (uint32_t)r});
    return target;
}

double execute(const vector<Instruction>& program, const vector<double>& constPool, const vector<double>& varValues, double* registers) {
    for (const auto& inst : program) {
        switch ((OpCode)inst.op) {
            case OpCode::MOV_CONST: registers[inst.resIdx]=constPool[inst.leftIdx]; 
                break;
            case OpCode::LOAD_VAR:  registers[inst.resIdx]=varValues[inst.leftIdx]; 
                break;
            case OpCode::ADD:       registers[inst.resIdx]=registers[inst.leftIdx]+registers[inst.rightIdx]; 
                break;
            case OpCode::SUB:       registers[inst.resIdx]=registers[inst.leftIdx]-registers[inst.rightIdx]; 
                break;
            case OpCode::MUL:       registers[inst.resIdx]=registers[inst.leftIdx]*registers[inst.rightIdx]; 
                break;
            case OpCode::DIV:       registers[inst.resIdx]=(registers[inst.rightIdx]!=0) ? registers[inst.leftIdx]/registers[inst.rightIdx] : 0; 
                break;
            case OpCode::HALT:      
                return registers[inst.resIdx];
        }
    }
    return 0;
}

void dumpProgram(const vector<Instruction>& program, const vector<double>& constPool, const map<string, int>& varMap) {
    map<int, string> revVarMap;
    for (auto const& [name, id] : varMap) 
        revVarMap[id]=name;
    for (int i=0; i<program.size(); ++i) {
        const auto& inst=program[i];
        cout<<"["<<setw(2)<<i<<"] "<<left<<setw(10)<<opNames[inst.op]<<" r"<<inst.resIdx<<", ";
        if ((OpCode) inst.op==OpCode::MOV_CONST) 
            cout<<constPool[inst.leftIdx];
        else if ((OpCode)inst.op==OpCode::LOAD_VAR) 
            cout<<revVarMap[inst.leftIdx];
        else if ((OpCode)inst.op==OpCode::HALT) 
            cout<<"exit";
        else 
            cout<<"r"<<inst.leftIdx<<", r"<<inst.rightIdx;
        cout<<endl;
    }
}

int main() {
    string input;
    cout<<"Enter: ";
    getline(cin, input);
    tokenize(input);
    auto root=parseExpr();
    if (root) {
        vector<Instruction> program;
        vector<double> constPool;
        map<string, int> varMap;
        int nextReg=0;
        int finalReg=compile(root.get(), program, constPool, nextReg, varMap);
        program.push_back({(uint32_t)OpCode::HALT, (uint32_t)finalReg, 0, 0});
        dumpProgram(program, constPool, varMap);
        cout<<"\nPress Enter for x=1.00 to 100.00...";
        cin.get();
        vector<double> varValues(varMap.size(), 0);
        double registers[1024]={0};
        cout<<fixed<<setprecision(2);
        for (int i=100; i<=10000; ++i) {
            double x=i/100.0;
            if (varMap.count("x")) 
                varValues[varMap["x"]]=x;
            double result=execute(program, constPool, varValues, registers);
            cout<<"x = "<<setw(6)<<x<<" | Result = "<<setw(10)<<setprecision(4)<<result<<setprecision(2)<<endl;
        }
    }
    return 0;
}