#include <sstream>
#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>
#include "vkSearch.hpp"
#include "cl.hpp"
#include "standardFunctions.hpp"
#include "implimentedFunctions.hpp"
#include "performance-analyzer/performance-analyzer.hpp"

std::string loadFile(std::string filename);
void printVector(const std::vector<int>& vec); 
int main() {

    std::string str = loadFile("500MB_random.txt");
    std::string toFind = "wj%|0fY41AN";
    int stringSearchManual{0}, standardLibFind{0}, vkSearchResult{0}, standardContainsResult{0}, standardFindAllResult{0};
    // Start the profiler
    Profiler::Get().BeginSession("Main", "../profiler-results.json");


    stringSearchManual = stringSearch(str, toFind);
    standardLibFind = standardFind(str, toFind);
    std::vector<int> openCLSearch = clSearch(str, toFind);
    //vkSearchResult = vulkanStringSearch(str, toFind);
    standardContainsResult = standardContains(str, toFind);
    standardFindAllResult = standardFindAll(str,toFind);

    // end the profiler
    Profiler::Get().EndSession();
    std::cout << "Location of string search manual: " << stringSearchManual << std::endl;
    std::cout << "Location of standard find: " << standardLibFind << std::endl;
    printVector(openCLSearch);
    std::cout << "Standard contains: " << standardContainsResult << std::endl;
    std::cout << "Contains # of standard find all: " << standardFindAllResult << std::endl;
    return 0;
}

std::string loadFile(std::string filename){
    std::ifstream t(filename);
    std::stringstream buffer;
    buffer << t.rdbuf();
    return (buffer.str());
}

 void printVector(const std::vector<int>& vec) {
    std::cout << "[ ";
    for (int val : vec) {
        std::cout << val << " ";
    }
    std::cout << "]\n";
}

