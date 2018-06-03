#include <iostream>
#include <vector>
#include <algorithm>
#include <utility>
#include <string>
#include "big_integer.h"

int main() {
    big_integer test("-12312312312581274981237984127394");
    test >>= 20;
    std::cout << test;
    return 0;
}