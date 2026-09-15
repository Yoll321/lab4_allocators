#include <iostream>
#include <map>

#include "my_allocator.hpp"
#include "my_list.hpp"

int factorial(int n) {
    std::uint64_t res = 1;
    for (std::uint64_t i = 2; i <= n; i++) {
        res *= i;
    }
    return res;
}

int main() {
    std::map<int, int> m1;
    for (int i = 0; i < 10; i++) {
        m1[i] = factorial(i);
    }
    std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
    std::cout << "Printing the contains of map with std::alloc:\n";
    std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
    for (auto p : m1) {
        std::cout << p.first << ") " << p.second << '\n';
    }

    std::map<int, int, std::less<int>, MyAlloc<std::pair<const int, int>, 10>> m2;
    for (int i = 0; i < 10; i++) {
        m2[i] = factorial(i);
    }
    std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
    std::cout << "Printing the contains of map with MyAlloc:\n";
    std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
    for (auto p : m2) {
        std::cout << p.first << ") " << p.second << '\n';
    }

    MyList<int> list1;
    for (int i = 0; i < 10; i++) {
        list1.push_back(i);
    }
    std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
    std::cout << "Printing the contains of list with std::alloc:\n";
    std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
    for (int i : list1) {
        std::cout << i << ' ';
    } std::cout << '\n';

    MyList<int, MyAlloc<int, 10>> list2;
    for (int i = 0; i < 10; i++) {
        list2.push_back(i);
    }
    std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
    std::cout << "Printing the contains of list with MyAlloc:\n";
    std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
    for (int i : list2) {
        std::cout << i << ' ';
    } std::cout << '\n';

    return 0;
}