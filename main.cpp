#include <iostream>
#include "performance-analyzer/Profiler.h"

extern "C" int containsASM(const char* s1, const char* s2);
int standardContains(const std::string &str, const std::string &subStr);
int stringSearch(const std::string &str, const std::string &subStr);
int standardFind(const std::string &str, const std::string &subStr);
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
    found2 = standardContains(word, item);
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



/*
 * standard contains function that returns 1 if the substring is found in the string, 0 otherwise
 *
 * @param s1 : the string to search in
 * @param s2 : the substring to search for
 *
 * @return 1 if the substring is found in the string, 0 otherwise
 */
 int standardContains(const std::string &str, const std::string &subStr) {
    PROFILE_FUNCTION();
    return str.find(subStr) != std::string::npos;
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
