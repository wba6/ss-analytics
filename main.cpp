#include <iostream>
#include "vendor/performance-analyzer/Profiler.h"

int stringSearch(const std::string &str, const std::string &subStr);
int main() {

    // Start the profiler
    Profiler::Get().BeginSession("Main", "profiler-results.json");
    stringSearch("Hello, World!", "World");

    // end the profiler
    Profiler::Get().EndSession();
    return 0;
}

/*
 * string search function that returns the index of the first occurrence of the substring in the string
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