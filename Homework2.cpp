#include <string>
#include <stack>
#include <cctype>
#include <iostream>
#include <map>

using namespace std;

int priority(char op)
{
    switch(op) {
        case '+':
        case '-':
            return 1;

        case '/':
        case '*':
            return 2;
        case '(':
            return 0;
        
        default:
            return -1;
    }
}


bool isOperator(char c) {
    return (c == '+' || c == '-' || c == '*' || c == '/' || c == '(' || c == ')');
}

bool isMathOperator(char c) {
    return (c == '+' || c == '-' || c == '*' || c == '/');
}

double compute(double a , double b ,char op){
    if (op == '+')
        return a + b;
    else if (op == '-')
        return a - b;
    else if(op == '*')
        return a * b;
    else 
        return a / b;
    }

string infixToPostfix(string expr)
{
    stack<char> operators;
    string postfix_exp;

    for(int i=0; i <expr.length(); i++)
    {
        if (isdigit(expr[i]))
        {
            while (i < expr.length() && isdigit(expr[i])){
                postfix_exp.push_back(expr[i]);
                i++;
            }
            postfix_exp.push_back(' ');
            i--;
        }
        else if (expr[i] == '(')
            operators.push(expr[i]);
        else if (expr[i] == ')'){
            while(!operators.empty() && operators.top() != '('){
                postfix_exp.push_back(operators.top());
                operators.pop();
            }
            if(!operators.empty())
                operators.pop();
        }
        else if (isOperator(expr[i])){
            while(!operators.empty() && priority(operators.top()) >= priority(expr[i])){

                postfix_exp.push_back(operators.top());
                operators.pop();
            }
            operators.push(expr[i]);
        }
    }
    while(!operators.empty()){
        postfix_exp.push_back(operators.top());
       operators.pop();
    }
    return postfix_exp;
}

double countPostfix(string postfix,map<char,double>& vars)
{
    stack<double> numbers;
    for(int i = 0;i < postfix.length(); i++){
        if(isdigit(postfix[i]))
        {
            double num = 0;
            while (i < postfix.length() && isdigit(postfix[i])){
                num = num *10 + (postfix[i] - '0');
                i++;
            }
            numbers.push(num);
        }
        else if (isalpha(postfix[i]))
        {
            char var = postfix[i];
            double value = vars[var]; 
            numbers.push(value);
        }
        else if (isMathOperator(postfix[i]))
        {
            double b = numbers.top();
            numbers.pop();
            double a  = numbers.top();
            numbers.pop();
            double result = compute(a, b, postfix[i]);
            numbers.push(result);
        }
    }
    return numbers.top();
}

int main(){
    string exp = "3*(2*10)";
    map<char,double> vars;
    vars['a'] = 3;
    vars['b'] = 4;
    cout<<"here is exp: "<<countPostfix(infixToPostfix(exp), vars);
    return 0;
}