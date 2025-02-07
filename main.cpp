#include <iostream>
#include <fstream>
#include "cl.hpp"
#include "standardFunctions.hpp"
#include "implimentedFunctions.hpp"
#include "performance-analyzer/Profiler.h"

extern "C" int containsASM(const char* s1, const char* s2);
std::string loadFile(std::string filename);
int main() {

    std::string str = loadFile("example.txt");
    std::string toFind = "hello";
    int location{0}, location2{0}, found{0}, found2{0};

    // Start the profiler
    Profiler::Get().BeginSession("Main", "profiler-results.json");

    location = stringSearch(str, toFind);
    location2 = standardFind(str, toFind);
    {
        PROFILE_SCOPE("ASM Search");
        found = containsASM(str.c_str(), toFind.c_str());
    }

    // end the profiler
    Profiler::Get().EndSession();
    std::cout << "Location: " << location << std::endl;
    std::cout << "Location2: " << location2 << std::endl;
    std::cout << "Found: " << found << std::endl;
    std::cout << "Found2: " << found2 << std::endl;
    return 0;
}

std::string loadFile(std::string filename){
    std::ifstream t(filename);
    t.seekg(0, std::ios::end);
    size_t size = t.tellg();
    std::string buffer(size, ' ');
    t.seekg(0);
    t.read(&buffer[0], size);
    return std::move(buffer);
}
