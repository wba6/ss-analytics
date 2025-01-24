#include <iostream>
#include "vendor/performance-analyzer/Profiler.h"


int stringSearch(const std::string &str, const std::string &subStr);
void function();
int standardFind(const std::string &str, const std::string &subStr);
int main() {

    // Start the profiler
    Profiler::Get().BeginSession("Main", "profiler-results.json");
    std::string word, item;
    getline(std::cin,word);
    std::cin >> item;

    //output used variables
    std::cout << "Word: " << word << std::endl;
    std::cout << "Item: " << item << std::endl;

    int location = stringSearch(word, item);
    //function();
    int location2 =standardFind(word, item);

    // end the profiler
    Profiler::Get().EndSession();
    std::cout << "Location: " << location << std::endl;
    std::cout << "Location2: " << location2 << std::endl;
    return 0;
}

/*
 * string search function that returns the index of the first occurrence of the substring in the string
 *
 * @param str : the string to search in
 * @param subStr : the substring to search for
 *
 * @return the index of the first occurrence of the substring in the string
 */
int stringSearch(const std::string &str, const std::string &subStr) {
    PROFILE_FUNCTION();

    int strLen = str.length();
    int subStrLen = subStr.length();
    for (int i = 0; i < strLen - subStrLen; i++) {
        int j = 0;
        while (j < subStrLen && str[i + j] == subStr[j]) {
            j++;
        }
        if (j == subStrLen) {
            return i;
        }
    }
    return -1;
}

/*
 * Standard string search function that returns the index of the first occurrence of the substring in the string
 *
 * @param str : the string to search in
 * @param subStr : the substring to search for
 *
 * @return the index of the first occurrence of the substring in the string
 */
int standardFind(const std::string &str, const std::string &subStr) {
    PROFILE_FUNCTION();
    return str.find(subStr);
}