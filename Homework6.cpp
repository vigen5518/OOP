#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <iomanip>
using namespace std;


enum OpCode {MOV, ADD, SUB, MUL, DIV, HALT};
string opNames[]={"MOV", "ADD", "SUB", "MUL", "DIV", "HALT"};

struct Instruction {
    OpCode op;
    int resIdx;    
    int leftIdx;   
    int rightIdx;  
    double literal; 
};

struct State {
    double registers[1024]={0};
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

enum NodeKind {NUM_NODE, VAR_NODE, OP_NODE};
struct ExprNode {
    NodeKind kind; 
    string text; 
    double val;
    ExprNode *left=nullptr, *right=nullptr;
    ExprNode(NodeKind k, string t) : kind(k), text(t), val(0) {}
    ExprNode(double v) : kind(NUM_NODE), text(""), val(v) {}
    ~ExprNode() { 
        delete left; 
        delete right; 
    }
};

ExprNode *parseExpr();
ExprNode *parsePrimary() {
    if (curIdx>=tokenList.size()) 
        return nullptr;
    Token t=tokenList[curIdx++];
    if (t.type==NUMBER) 
        return new ExprNode(stod(t.value));
    if (t.type==VARIABLE) 
        return new ExprNode(VAR_NODE, t.value);
    if (t.type==LPAREN) {
        ExprNode *n=parseExpr();
        if (curIdx<tokenList.size()) 
            curIdx++; 
        return n;
    }
    return nullptr;
}

ExprNode *parseTerm() {
    ExprNode *l=parsePrimary();
    while (curIdx<tokenList.size() && (tokenList[curIdx].value=="*" || tokenList[curIdx].value=="/")) {
        string op=tokenList[curIdx++].value;
        ExprNode *p=new ExprNode(OP_NODE, op);
        p->left=l; 
        p->right=parsePrimary();
        l=p;
    }
    return l;
}

ExprNode *parseExpr() {
    ExprNode *l=parseTerm();
    while (curIdx<tokenList.size() && (tokenList[curIdx].value=="+" || tokenList[curIdx].value=="-")) {
        string op=tokenList[curIdx++].value;
        ExprNode *p=new ExprNode(OP_NODE, op);
        p->left=l; 
        p->right=parseTerm();
        l=p;
    }
    return l;
}

int compile(ExprNode *node, vector<Instruction> &program, int &nextReg, map<string, int> &varMap) {
    if (!node) 
        return -1;
    if (node->kind==NUM_NODE) {
        int r=nextReg++;
        program.push_back({MOV, r, -1, -1, node->val});
        return r;
    }
    if (node->kind==VAR_NODE) {
        if (varMap.find(node->text)==varMap.end()) 
            varMap[node->text]=nextReg++;
        return varMap[node->text];
    }
    int l=compile(node->left, program, nextReg, varMap);
    int r=compile(node->right, program, nextReg, varMap);
    int target=nextReg++;
    OpCode op=(node->text=="+") ? ADD : (node->text=="-") ? SUB : (node->text=="*") ? MUL : DIV;
    program.push_back({op, target, l, r, 0});
    return target;
}

double execute(const vector<Instruction> &program, State &vm) {
    for (const auto &inst : program) {
        switch (inst.op) {
            case MOV:  vm.registers[inst.resIdx]=inst.literal; 
                break;
            case ADD:  vm.registers[inst.resIdx]=vm.registers[inst.leftIdx]+vm.registers[inst.rightIdx]; 
                break;
            case SUB:  vm.registers[inst.resIdx]=vm.registers[inst.leftIdx]-vm.registers[inst.rightIdx]; 
                break;
            case MUL:  vm.registers[inst.resIdx]=vm.registers[inst.leftIdx]*vm.registers[inst.rightIdx]; 
                break;
            case DIV:  vm.registers[inst.resIdx]=(vm.registers[inst.rightIdx]!=0) ? vm.registers[inst.leftIdx]/vm.registers[inst.rightIdx] : 0; 
                break;
            case HALT: return vm.registers[inst.resIdx];
        }
    }
    return 0;
}

int main() {
    string input;
    cout<<"Enter: ";
    getline(cin, input);

    tokenize(input);
    ExprNode *root=parseExpr();

    if (root) {
        State vm;
        vector<Instruction> program;
        map<string, int> varMap;
        int nextReg=0;

        int finalReg=compile(root, program, nextReg, varMap);
        program.push_back({HALT, finalReg, 0, 0, 0});

        for (const auto &inst : program) {
            cout<<left<<setw(6)<<opNames[inst.op]<<" r"<<inst.resIdx;
            if (inst.op==MOV) 
                cout<<", "<<inst.literal<<endl;
            else if (inst.op==HALT) 
                cout<<" (Final result in r"<<inst.resIdx<<")"<<endl;
            else cout<<", r"<<inst.leftIdx<<", r"<<inst.rightIdx<<endl;
        }

        cout<<"\nPress Enter to execute the program and see results for x=1.00 to 100.00...";
        cin.ignore();
        cin.get();

        cout<<"\nResults:\n";
        cout<<fixed<<setprecision(2);
        
        for (int i=100; i<=10000; ++i) {
            double x=i/100.0;
            if (varMap.count("x"))
                vm.registers[varMap["x"]]=x;

            double result=execute(program, vm);
            cout<<"x = "<<setw(6)<<x<<" | Result = "<<setw(10)<<setprecision(4)<<result<<setprecision(2)<<endl;
        }
    } 
    else
        cout<<"Error: Invalid expression."<<endl;

    delete root;
    return 0;
}