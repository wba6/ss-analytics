#include <sstream>
#include <cstring>
#include <iostream>
#include <fstream>
#include "cl.hpp"
#include "standardFunctions.hpp"
#include "implimentedFunctions.hpp"
#include "performance-analyzer/Profiler.h"

std::string loadFile(std::string filename);
int main() {

    std::string str = loadFile("500MB_random.txt");
    std::string toFind = "belkjdalfjlkj;sdf";
    int location{0}, location2{0},location3{0}, found{0}, found2{0};
    // Start the profiler
    Profiler::Get().BeginSession("Main", "profiler-results.json");

    location = stringSearch(str, toFind);
    location2 = standardFind(str, toFind);
    location3 = clSearch(str, toFind);
    found2 = standardContains(str, toFind);

    // end the profiler
    Profiler::Get().EndSession();
    std::cout << "Location: " << location << std::endl;
    std::cout << "Location2: " << location2 << std::endl;
    std::cout << "Location3: " << location3 << std::endl;
    std::cout << "Found2: " << found2 << std::endl;
    return 0;
}

std::string loadFile(std::string filename){
    std::ifstream t(filename);
    std::stringstream buffer;
    buffer << t.rdbuf();
    return (buffer.str());
}

