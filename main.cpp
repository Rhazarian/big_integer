#include <iostream>
#include <vector>
#include <algorithm>
#include <utility>
#include <string>
#include "big_integer.h"

int main() {
    big_integer a = std::numeric_limits<int>::min();
    big_integer b = -a;
    big_integer mone = -1;
    big_integer c = b + mone;
    std::cout << b - 1;
    return 0;
}