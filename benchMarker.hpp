/*
 * benchMaker.hpp is the include file for the benchMaker class which is used
 * to run bench marks for various searching algorithms at different file sizes.
 * 
 */

#ifndef BENCHMARKER_HPP
#define BENCHMARKER_HPP
#include <string>
#include <functional>
#include <vector>

class BenchMarker {
public:
    /**
    * @brief Constructs a BenchMarker instance.
    *
    * Initializes the BenchMaker with vectors of function objects for single and multiple integer returns,
    * a vector of test file sizes (in MB), and sets the default test data file name.
    *
    * @param singleReturn Vector of functions that return an integer given two strings.
    * @param multiReturn Vector of functions that return a vector of integers given two strings.
    * @param testSizes Vector of test file sizes (in megabytes) to be used during benchmarking.
    */
    BenchMarker(std::vector<std::function<int(std::string&, std::string&)>>& singleReturn,
                std::vector<std::function<std::vector<int>(std::string&, std::string&)>>& multiReturn,
                std::vector<unsigned int> testSizes);

    ~BenchMarker();

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
    bool runBenchmark(std::string& outputFilePrefix, std::string& testDataFileName);

private:
    /**
    * @brief Loads the entire content of a file into a string.
    *
    * Opens the specified file in text mode, reads its entire content, and returns it as a std::string.
    *
    * @param fileName Name of the file to read.
    * @return A std::string containing the contents of the file.
    */
    std::string loadStringFromFile(std::string& fileName);

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
    bool generateFile(unsigned int fileSizeMB, std::string& substring, unsigned int occurances); 
    
    /**
    * @brief Executes a set of functions with consistent output verification.
    *
    * This templated function iterates over a vector of functions (which should all take two std::string references as input)
    * and executes them on the given data and substring. It assumes that each function returns a result comparable with the
    * result of the first function in the vector. If any subsequent function returns a different result, a std::runtime_error
    * is thrown indicating inconsistent behavior.
    *
    * @tparam T The type of function in the vector. It must be callable with (std::string&, std::string&) and return a comparable type.
    * @param vec The vector of functions to execute.
    * @param data The input data string to be processed.
    * @param substring The substring used in processing the data.
    *
    * @throws std::runtime_error if any function returns a result different from the first function's output.
    */
    template<typename T>
    void runFunctions(const std::vector<T>& vec, std::string& data, std::string& substring); 

private:
    std::vector<std::function<int(std::string&, std::string&)>>& m_singleReturnVec;
    std::vector<std::function<std::vector<int>(std::string&, std::string&)>>& m_multiReturnVec;
    std::vector<unsigned int> m_testSizes;
    std::string m_testDataFileName;
};

#endif // BENCHMARKER_HPP
