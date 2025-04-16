/**
 * @file benchMarker.cpp
 * @brief Implementation file for the BenchMarker class.
 *
 * This file contains the implementation for benchmarking functions that operate on test data files.
 */

#include "benchMarker.hpp"
#include <fstream>
#include <sstream>
#include <random>
#include <string>
#include <stdexcept>
#include <vector>
#include <cstdio>
#include <iostream>
#include "performance-analyzer/performance-analyzer.hpp"

BenchMarker::BenchMarker(std::vector<std::function<int(std::string&, std::string&)>>& singleReturn,
                std::vector<std::function<std::vector<int>(std::string&, std::string&)>>& multiReturn,
                std::vector<unsigned int> testSizes)
:m_singleReturnVec(singleReturn), m_multiReturnVec(multiReturn), m_testSizes(testSizes), m_testDataFileName("testData.txt"){};

BenchMarker::~BenchMarker() {
    std::remove(m_testDataFileName.c_str()); // delete file
 
    if (std::ifstream{m_testDataFileName}) // uses operator! of temporary stream object
    {
        std::cerr << "Error deleted file test file" << std::endl;
    }
}

/**
 * @brief Runs the benchmark tests.
 *
 * For each file size specified in m_testSizes, this function generates a test file with random data
 * and inserted substring occurrences, loads the file content, and then runs the functions in both
 * m_singleReturnVec and m_multiReturnVec while profiling their performance.
 * The profiling results are written to a JSON file named using the provided prefix and file size.
 *
 * @param outputFilePrefix Prefix for the output JSON file containing benchmark results.
 * @param testDataFileName Name of the file used for test data generation and reading.
 * @return true if the benchmark completes successfully.
 */
bool BenchMarker::runBenchmark(std::string& outputFilePrefix, std::string& testDataFileName) {
    m_testDataFileName = testDataFileName;    

    std::string substring = "akdl;jfksjft";
    for(auto size: m_testSizes) {
        std::cout << "Running test with file size: " << size << "MB" << std::endl;
        generateFile(size, substring, 10);

        std::string data = loadStringFromFile(m_testDataFileName);

        std::string outputFileName = outputFilePrefix + "_" + std::to_string(size);
        outputFileName += "MB.json";

        Profiler::Get().BeginSession("BenchMarker", outputFileName);

        runFunctions(m_singleReturnVec, data, substring);
        runFunctions(m_multiReturnVec, data, substring);

        Profiler::Get().EndSession();
        
        std::cout << "Finished test with file size: " << size << "MB\n";
        std::cout << "Results from test outputed into " << outputFileName << "\n" << std::endl; 
    }

    return true;
}


/**
 * @brief Loads the entire content of a file into a string.
 *
 * Opens the specified file in text mode, reads its entire content, and returns it as a std::string.
 *
 * @param fileName Name of the file to read.
 * @return A std::string containing the contents of the file.
 */
std::string BenchMarker::loadStringFromFile(std::string& fileName) {
    std::ifstream t(fileName);
    std::stringstream buffer;
    buffer << t.rdbuf();
    return (buffer.str());
}


/**
 * @brief Generates a test file with random data and specific substring occurrences.
 *
 * Generates a file with the specified size (in MB) filled with random printable ASCII characters.
 * Inserts a given substring a specified number of times at random positions within the file.
 * The resulting content is written to the file specified by m_testDataFileName.
 *
 * @param fileSizeMB Size of the file to generate in megabytes.
 * @param substring The substring to insert into the file.
 * @param occurrences The number of times the substring should appear in the file.
 * @return true if the file is generated and written successfully.
 *
 * @throws std::invalid_argument if the substring is empty or if the file size is insufficient to contain the specified number of substring occurrences.
 * @throws std::runtime_error if the file cannot be opened for writing.
 */
bool BenchMarker::generateFile(unsigned int fileSizeMB, std::string& substring, unsigned int occurrences) {
    // Convert file size from MB to bytes
    unsigned int fileSize = fileSizeMB * 1024 * 1024;

    // checks
    if (substring.empty()) {
        throw std::invalid_argument("Substring cannot be empty.");
    }

    if (fileSize < substring.size() * occurrences) {
        throw std::invalid_argument("File size is too small to accommodate all occurrences of the substring.");
    }

    // Prepare a buffer to hold the file content
    std::string buffer(fileSize, '\0');

    // Random engine and distribution
    std::random_device rd;
    std::mt19937 gen(rd());
    //  printable ASCII characters roughly in the range [32, 126]
    std::uniform_int_distribution<int> dist(32, 126);

    // fill the entire buffer with random characters
    for (std::size_t i = 0; i < fileSize; ++i) {
        buffer[i] = static_cast<char>(dist(gen));
    }

    // insert the substring occurrences at random positions
    // choose positions such that each substring fits entirely
    std::uniform_int_distribution<std::size_t> positionDist(0, fileSize - substring.size());

    for (std::size_t i = 0; i < occurrences; ++i) {
        std::size_t pos = positionDist(gen);
        // Copy substring into the buffer
        for (std::size_t j = 0; j < substring.size(); ++j) {
            buffer[pos + j] = substring[j];
        }
    }

    // write the buffer to the specified file
    std::ofstream outFile(m_testDataFileName, std::ios::binary);
    if (!outFile) {
        throw std::runtime_error("Failed to open file: " + m_testDataFileName);
    }
    outFile.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    outFile.close();

    return true;
}

template<typename T>
void BenchMarker::runFunctions(const std::vector<T>& vec, std::string& data, std::string& substring) {
    // Assume the return type is std::vector<int>
    if (!vec.empty()) {
        // Call the first function and store its return value.
        auto expected = vec.front()(data, substring);
        
        // Iterate through the remaining functions.
        for (size_t i = 1; i < vec.size(); ++i) {
            auto result = vec[i](data, substring);
            if (result != expected) {
                throw std::runtime_error("Inconsistent return value detected at function index " + std::to_string(i));
            }
        }
    }
}

