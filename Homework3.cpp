#include <iostream>
#include <vector>
#include <map>
using namespace std;

enum TokenType {NUMBER, VARIABLE, OPERATOR, LPAREN, RPAREN, END};

struct Token {
    TokenType type;
    string value;
};

vector<Token> tokens;
int current_index=0;
map<string, double> memory;
double expression();

void tokenize(string input) {
    tokens.clear();
    for (int i=0; i<input.length(); ++i) {
        char c=input[i];

        if (isspace(c)) 
            continue; 

        if (isdigit(c)) { 
            string num;
            while (i<input.length() && (isdigit(input[i]) || input[i]=='.')) {
                num+=input[i++];
            }
            i--;
            tokens.push_back({NUMBER, num});
        } 
        else if (isalpha(c)) { 
            string var;
            while (i<input.length() && isalnum(input[i])) {
                var+=input[i++];
            }
            i--;
            tokens.push_back({VARIABLE, var});
        } 
        else { 
            TokenType t=OPERATOR;
            if (c=='(') 
                t=LPAREN;
            else if (c==')') 
                t=RPAREN;
            tokens.push_back({t, string(1, c)});
        }
    }
    tokens.push_back({END, ""});
}

double value() {
    Token t=tokens[current_index++];
    
    if (t.type==NUMBER) 
        return stod(t.value);
    if (t.type==VARIABLE) 
        return memory[t.value];
    if (t.type==LPAREN) {
        double result=expression();
        if (tokens[current_index].type==RPAREN) 
            current_index++; 
        return result;
    }
    return 0;
}

double multi() {
    double result=value();
    while (tokens[current_index].type==OPERATOR && (tokens[current_index].value=="*" || tokens[current_index].value=="/")) {
        string op=tokens[current_index++].value;
        if (op=="*") 
            result*=value();
        else 
            result/=value();
    }
    return result;
}

double expression() {
    double result=multi();
    while (tokens[current_index].type==OPERATOR && (tokens[current_index].value=="+" || tokens[current_index].value=="-")) {
        string op=tokens[current_index++].value;
        if (op=="+") 
            result+=multi();
        else 
            result-=multi();
    }
    return result;
}

int main() {
    string input;
    cout<<"Enter an expression:";
    getline(cin, input);

    tokenize(input);

    for (double x=0; x<=100; x+=0.01) {
        memory["x"]=x; 
        current_index=0; 
        
        double final_result=expression();
        
        cout<<final_result<<"\n";
    }
    return 0;
}