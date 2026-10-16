#include <iostream>

#include "ast.h"
#include "parser.h"

int main()
{
        std::string input;

        std::getline(std::cin, input);
        
        ExprPtr expr = parse_text(input);

        expr->print();
        std::cout << '\n';

        return 0;
}