#include <iostream>
#include <vector>
#include <string>
#include <map>
using namespace std;

enum TokenType {NUMBER, VARIABLE, OPERATOR, LPAREN, RPAREN, END};

struct Token {
    TokenType type;
    string value;
};

enum NodeKind {NUM_NODE, VAR_NODE, OP_NODE};

struct ExprNode {
    NodeKind kind;
    string text;
    double number;

    ExprNode* left=nullptr;
    ExprNode* right=nullptr;

    ExprNode(NodeKind k, string t) {
        kind=k;
        text=t;
    }

    ExprNode(double val) {
        kind=NUM_NODE;
        number=val;
    }

    ~ExprNode() {
        delete left;
        delete right;
    }
};

vector<Token> tokenList;
int currentIndex=0;

ExprNode* parseExpression();

ExprNode* parsePrimary() {
    Token tok = tokenList[currentIndex];
    currentIndex++;

    if(tok.type==NUMBER) {
        double num=stod(tok.value);
        return new ExprNode(num);
    }

    if(tok.type==VARIABLE) {
        return new ExprNode(VAR_NODE, tok.value);
    }

    if(tok.type==LPAREN) {
        ExprNode* inside=parseExpression();
        if(tokenList[currentIndex].type==RPAREN) 
            currentIndex++;
        return inside;
    }
    return nullptr;
}

ExprNode* parseTerm() {
    ExprNode* leftSide=parsePrimary();
    while(currentIndex<tokenList.size()) {
        Token tok=tokenList[currentIndex];

        if(tok.type!=OPERATOR || (tok.value!="*" && tok.value!="/"))
            break;
        currentIndex++;

        ExprNode* parent=new ExprNode(OP_NODE, tok.value);
        parent->left=leftSide;

        ExprNode* rightSide=parsePrimary();
        parent->right=rightSide;

        leftSide=parent;
    }
    return leftSide;
}

ExprNode* parseExpression() {
    ExprNode* leftSide=parseTerm();
    while(currentIndex<tokenList.size()) {
        Token tok=tokenList[currentIndex];

        if(tok.type!=OPERATOR || (tok.value!="+" && tok.value!="-"))
            break;
        currentIndex++;

        ExprNode* parent=new ExprNode(OP_NODE, tok.value);
        parent->left=leftSide;
        parent->right=parseTerm();

        leftSide=parent;
    }
    return leftSide;
}

double evalTree(ExprNode* node, map<string,double>& vars) {
    if(node==nullptr)
        return 0;

    if(node->kind==NUM_NODE)
        return node->number;

    if(node->kind==VAR_NODE)
        return vars[node->text];

    double leftVal=evalTree(node->left, vars);
    double rightVal=evalTree(node->right, vars);

    if(node->text=="+") 
        return leftVal+rightVal;
    if(node->text=="-") 
        return leftVal-rightVal;
    if(node->text=="*") 
        return leftVal*rightVal;
    if(node->text=="/") {
        if(rightVal==0) 
            return 0;
        return leftVal/rightVal;
    }
    return 0;
}

void tokenizeInput(string input) {
    tokenList.clear();
    currentIndex=0;

    for(int i=0; i<input.length(); ++i) {
        char c=input[i];

        if(isspace(c))
            continue;
        if(isdigit(c)) {
            string temp;
            while(i<input.length() && (isdigit(input[i]) || input[i]=='.')) {
                temp+=input[i];
                i++;
            }
            tokenList.push_back({NUMBER, temp});
            i--;
        }
        else if(isalpha(c)) {
            string temp;
            while(i<input.length() && isalnum(input[i])) {
                temp+=input[i];
                i++;
            }
            tokenList.push_back({VARIABLE, temp});
            i--;
        }
        else if(c=='(')
            tokenList.push_back({LPAREN, "("});
        else if(c == ')')
            tokenList.push_back({RPAREN, ")"});
        else {
            string s(1, c);
            tokenList.push_back({OPERATOR, s});
        }
    }
    tokenList.push_back({END, ""});
}

int main() {
    string expr;
    cout<<"Enter: ";
    getline(cin, expr);
    tokenizeInput(expr);

    ExprNode* root=parseExpression();
    map<string,double> variables;

    for(double x=0; x<=100; x+=0.1) {
        variables["x"]=x;
        double result=evalTree(root, variables);
        cout<<result<<endl;
    }
    delete root;
    return 0;
}