#include <functional>
#include <cstring>
#include <string>
#include <vector>
#include "searchFunctions/cl.hpp"
#include "searchFunctions/standardFunctions.hpp"
#include "searchFunctions/implementedFunctions.hpp"
#include "performance-analyzer/performance-analyzer.hpp"

int main() {

    std::vector<std::function<int(std::string&, std::string&)>> benchMarkedSingleReturn {
        stringSearch,
        standardFind
    };    

    std::vector<std::function<std::vector<int>(std::string&,std::string&)>> benchMarkedMultiReturn { 
        clSearch,
        standardFindAll
    };

    std::vector<unsigned int> benchMarkFileSizes {
        10,
        50,
        100,
        500,
        800,
        1000,
        1200,
        1500
    };

    BenchMaker benchMarker(benchMarkedSingleReturn, benchMarkedMultiReturn, benchMarkFileSizes);

    std::string filePrefix = "../results/testOutput";
    std::string testDataName = "testData.txt";
    benchMarker.runBenchmark(filePrefix, testDataName);

    return 0;
}

