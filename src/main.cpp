#include <iostream>

#include "ast.h"
#include "parser.h"

int main()
{
        std::string input;

        std::cin >> input;
        
        ExprPtr expr = parse_text(input);

        return 0;
}