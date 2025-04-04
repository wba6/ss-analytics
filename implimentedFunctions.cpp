#include <string>
#include "performance-analyzer/performance-analyzer.hpp"
#include "implimentedFunctions.hpp"

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
