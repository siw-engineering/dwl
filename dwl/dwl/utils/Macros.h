#include <cstdio>
#include <iostream>

#define RED_     "\033[1m\033[31m"
#define GREEN_   "\033[32m"
#define YELLOW_  "\033[1m\033[33m"
#define BLUE_    "\033[1m\033[34m"
#define MAGENTA_ "\x1b[35m"
#define CYAN_    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"
#define PRINT_SCALAR(x) std::cout << #x " = " << x << "\n" << std::endl
#define PRINT_VECTOR(x) std::cout << #x " = ";\
	for (int i = 0; i < x.size(); i++) {std::cout << x[i] << " ";}\
	std::cout << std::endl;
