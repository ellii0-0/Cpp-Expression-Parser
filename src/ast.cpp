#include "ast.h"

#include <iostream>

bool binop_info(char op, OpInfo &info)
{
        switch (op) {
        case '+':
                info = {'+', 1, false};
                return true;
        case '-':
                info = {'-', 1, false};
                return true;
        case '*':
                info = {'*', 2, false};
                return true;
        case '/':
                info = {'/', 2, false};
                return true;
        case '^':
                info = {'^', 4, true};
                return true;
        default:
                return false;
        }
}

void Variable::print() const
{
        std::cout << name;
}

void Number::print() const
{
        std::cout << val;
}

void BinOp::print() const
{
        std::cout << '(';
        lhs->print();
        std::cout << op;
        rhs->print();
        std::cout << ')';
}

void UnOp::print() const
{
        std::cout << '(' << op;
        rhs->print();
        std::cout << ')';
}

void Call::print() const
{
        std::cout << name << '(';

        for (std::size_t i = 0; i < args.size(); i++) {
                if (i > 0)
                        std::cout << ", ";

                args[i]->print();
        }

        std::cout << ')';
}

void FuncDef::print() const
{
        std::cout << name << '(';

        for (std::size_t i = 0; i < params.size(); i++) {
                if (i > 0)
                        std::cout << ",";

                std::cout << params[i];
        }

        std::cout << ")=";
        body->print();
}
