#include <iostream>
#include <vector>

#include "safe_ranges.cpp"

int main() {
    std::cout << "Hello, World!" << std::endl;
    auto range = safe_ranges::range(std::vector{1, 2, 3, 4, 5, 6, 7, 8, 9});
    try{
        for (auto it = range.begin(); true; it++) {
            std::cout << *it << std::endl;
        }
    }
    catch(const std::out_of_range& e) {
        std::cerr << e.what() << std::endl;
    }
    try{
        auto it = range.begin();
        range.get().resize(1000);
        std::cout << *it << std::endl;
    }
    catch(const std::out_of_range& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
