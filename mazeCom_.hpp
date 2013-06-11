#ifndef MAZECOM__HPP
#define MAZECOM__HPP

#include <iostream>

//resolve conflict between WinError.h and mazeCom.hpp
#undef ERROR
#undef NOERROR

#include "mazeCom.hpp"

void print(std::ostream& os, boardType& b);

#endif