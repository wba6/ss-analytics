/*
 * benchMaker.hpp is the include file for the benchMaker class which is used
 * to run bench marks for various searching algrithms at different file sizes.
 * 
 */

#ifndef BENCHMARKER_HPP
#define BENCHMARKER_HPP
#include <string>
#include <functional>
#include <vector>

class BenchMaker {
public:
    BenchMaker(std::vector<std::function<int(std::string&, std::string&)>>& singleReturn,
                std::vector<std::function<std::vector<int>(std::string&, std::string&)>>& multiReturn,
                std::vector<unsigned int> testSizes);

    bool runBenchmark(std::string& outputFilePrefix, std::string& testDataFileName);

private:
    std::string loadStringFromFile(std::string& fileName);
    bool generateFile(unsigned int fileSizeMB, std::string& substring, unsigned int occurances); 
    
    template<typename T>
    void runFunctions(const std::vector<T>& vec, std::string& data, std::string& substring); 

private:
    std::vector<std::function<int(std::string&, std::string&)>>& m_singleReturnVec;
    std::vector<std::function<std::vector<int>(std::string&, std::string&)>>& m_multiReturnVec;
    std::vector<unsigned int> m_testSizes;
    std::string m_testDataFileName;
};

#endif // BENCHMARKER_HPP
