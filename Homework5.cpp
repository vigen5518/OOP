#include <iostream>
#include <vector>
#include <string>
#include <map>
using namespace std;

enum TokenType {NUMBER,VARIABLE,OPERATOR,LPAREN,RPAREN,END};

struct Token {
    TokenType type;
    string value;
};

enum NodeKind {NUM_NODE,VAR_NODE,OP_NODE};

struct ExprNode {
    NodeKind kind;
    string text;
    double number;
    ExprNode* left=nullptr;
    ExprNode* right=nullptr;

    ExprNode(NodeKind k,string t) : kind(k),text(t),number(0){}
    ExprNode(double val) : kind(NUM_NODE),text(""),number(val){}

    ~ExprNode(){
        delete left;
        delete right;
    }
};

struct Instruction {
    string target; 
    string left;
    string op;
    string right;
};

vector<Instruction> instructions;
vector<Token> tokenList;
int currentIndex=0;
int tempCounter=1;

string cleanNumber(double n){
    string s=to_string(n);
    if(s.find('.')!=string::npos){
        s.erase(s.find_last_not_of('0')+1, string::npos);
        if(s.back()=='.')
            s.pop_back();
    }
    return s;
}

string generateCode(ExprNode* node){
    if(!node)
        return "";
    if(node->kind==NUM_NODE)
        return cleanNumber(node->number);
    if(node->kind==VAR_NODE)
        return node->text;

    string leftOp=generateCode(node->left);
    string rightOp=generateCode(node->right);

    string target="tmp"+to_string(tempCounter++);
    instructions.push_back({target,leftOp,node->text,rightOp});

    return target;
}

ExprNode* parseExpression();

ExprNode* parsePrimary(){
    if(currentIndex>=tokenList.size())
        return nullptr;
    Token tok=tokenList[currentIndex++];

    if(tok.type==NUMBER)
        return new ExprNode(stod(tok.value));
    if(tok.type==VARIABLE)
        return new ExprNode(VAR_NODE,tok.value);
    if(tok.type==LPAREN){
        ExprNode* inside=parseExpression();
        if(currentIndex<tokenList.size() && tokenList[currentIndex].type==RPAREN)
            currentIndex++;
        return inside;
    }
    return nullptr;
}

ExprNode* parseTerm(){
    ExprNode* leftSide=parsePrimary();
    while(currentIndex<tokenList.size()){
        Token tok=tokenList[currentIndex];
        if(tok.type!=OPERATOR || (tok.value!="*" && tok.value!="/"))
            break;
        currentIndex++;
        ExprNode* parent=new ExprNode(OP_NODE,tok.value);
        parent->left=leftSide;
        parent->right=parsePrimary();
        leftSide=parent;
    }
    return leftSide;
}

ExprNode* parseExpression(){
    ExprNode* leftSide=parseTerm();
    while (currentIndex<tokenList.size()){
        Token tok=tokenList[currentIndex];
        if (tok.type!=OPERATOR || (tok.value!="+" && tok.value!="-"))
            break;
        currentIndex++;
        ExprNode* parent=new ExprNode(OP_NODE,tok.value);
        parent->left=leftSide;
        parent->right=parseTerm();
        leftSide=parent;
    }
    return leftSide;
}

void tokenizeInput(string input){
    tokenList.clear();
    currentIndex=0;
    for (int i=0; i<input.length(); ++i){
        char c=input[i];
        if(isspace(c))
            continue;
        if(isdigit(c)){
            string temp;
            while (i<input.length() && (isdigit(input[i]) || input[i]=='.')) 
                temp+=input[i++];
            tokenList.push_back({NUMBER,temp});
            i--;
        }
        else if(isalpha(c)){
            string temp;
            while (i<input.length() && isalnum(input[i])) 
                temp+=input[i++];
            tokenList.push_back({VARIABLE,temp});
            i--;
        }
        else{
            TokenType t=(c=='(') ? LPAREN : (c==')') ? RPAREN : OPERATOR;
            tokenList.push_back({t,string(1,c)});
        }
    }
    tokenList.push_back({END,""});
}

double evaluateFromVector(const vector<Instruction>& insts,map<string,double>& vars){
    map<string,double> temps;
    auto getVal=[&](string s){
        if (isdigit(s[0]) || (s.size()>1&&s[0]=='-'))
            return stod(s);
        if (temps.count(s)) 
            return temps[s];
        if (vars.count(s))
            return vars[s];
        return 0.0;
    };

    for(const auto& inst:insts){
        double v1=getVal(inst.left);
        double v2=getVal(inst.right);
        double res=0;
        if (inst.op=="+")
            res=v1+v2;
        else if (inst.op=="-")
            res=v1-v2;
        else if (inst.op=="*")
            res=v1*v2;
        else if (inst.op=="/")
            res=(v2!=0)?v1/v2:0;
        temps[inst.target]=res;
    }

    if(insts.empty())return 0;
    return temps[insts.back().target];
}

int main(){
    string expr;
    cout<<"Enter: ";
    getline(cin,expr);

    tokenizeInput(expr);
    ExprNode* root=parseExpression();

    if(root){
        instructions.clear();
        tempCounter=1;
        generateCode(root);

        map<string,double> variables;
        for(double x=1.0;x<=100.0;x+=0.1){
            variables["x"]=x;
            double result=evaluateFromVector(instructions,variables);
            cout<<result<<endl;
        }
    }
    delete root;
    return 0;
}