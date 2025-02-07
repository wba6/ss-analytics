#include "standardFunctions.hpp"
#include "performance-analyzer/Profiler.h"
#include <string>

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
    return str.contains(subStr); ;
 }
