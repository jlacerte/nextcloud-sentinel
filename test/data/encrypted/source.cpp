#include <iostream>
#include <vector>
#include <string>

int main() {
    std::vector<std::string> items = {"apple", "banana", "cherry"};
    for (const auto& item : items) {
        std::cout << "Item: " << item << std::endl;
    }
    return 0;
}
