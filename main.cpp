#include <iostream>
#include "cl.hpp"
#include "standardFunctions.hpp"
#include "implimentedFunctions.hpp"
#include "performance-analyzer/Profiler.h"

extern "C" int containsASM(const char* s1, const char* s2);
int main() {


    int found = -1;
    int found2 = -1;
    int location = -1;
    int location2 = -1;

    std::string word, item;
    getline(std::cin,word);
    std::cin >> item;

    //output used variables
    std::cout << "Word: " << word << std::endl;
    std::cout << "Item: " << item << std::endl;

    //output used variables
    std::cout << "Word: " << word << std::endl;
    std::cout << "Item: " << item << std::endl;

    // Start the profiler
    Profiler::Get().BeginSession("Main", "profiler-results.json");

    location = stringSearch(word, item);
    location2 = standardFind(word, item);
    {
        PROFILE_SCOPE("ASM Search");
        found = containsASM(word.c_str(), item.c_str());
    }

    // end the profiler
    Profiler::Get().EndSession();
    std::cout << "Location: " << location << std::endl;
    std::cout << "Location2: " << location2 << std::endl;
    std::cout << "Found: " << found << std::endl;
    std::cout << "Found2: " << found2 << std::endl;
    return 0;
}
