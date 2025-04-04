#include "standardFunctions.hpp"
#include "performance-analyzer/performance-analyzer.hpp"
#include <vector>
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


/*
 * Standard string search function that returns the number occurrences of a substring 
 *
 * @param str : the string to search in
 * @param subStr : the substring to search for
 *
 * @return the occurrences of a substring
 */
std::vector<int> standardFindAll(const std::string &str, const std::string &subStr) {
    PROFILE_FUNCTION();
   // Find the first occurrence of target in text
    size_t pos = str.find(subStr);

    std::vector<int> occurrences;
    // Continue finding until no more occurrences are found
    while (pos != std::string::npos) {
        occurrences.push_back(pos); 
        // Find the next occurrence, starting just after the current one
        pos = str.find(subStr, pos + 1);
    }
    
    return std::move(occurrences);
}

