#include "read_input_functions.h"
#include <iostream>
std::string ReadInputFunctions::ReadLine()
{
    std::string s;
    getline(std::cin, s);
    return s;
}

int ReadInputFunctions::ReadLineWithNumber()
{
    int result;
    std::cin >> result;
    ReadLine();
    return result;
}