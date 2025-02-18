#include <sstream>
#include <cstring>
#include <iostream>
#include <fstream>
#include <xmmintrin.h> //SSE
#include <emmintrin.h> //SSE 2
#include <immintrin.h> //AVX
#include "cl.hpp"
#include "standardFunctions.hpp"
#include "implimentedFunctions.hpp"
#include "performance-analyzer/Profiler.h"

int stringSearchASM(const char* s1, const char* s2);
std::string loadFile(std::string filename);
int main() {

    std::string str = loadFile("500MB_random.txt");
    std::string toFind = "?t1Cns5y:Uu_5*K60#M";
    int location{0}, location2{0},location3{0}, found{0}, found2{0};
    // Start the profiler
    Profiler::Get().BeginSession("Main", "profiler-results.json");

    location = stringSearch(str, toFind);
    location2 = standardFind(str, toFind);
    location3 = clSearch(str, toFind);
    {
        PROFILE_SCOPE("ASM Search");
        found = stringSearchASM(str.c_str(), toFind.c_str());
    }
    found2 = standardContains(str, toFind);

    // end the profiler
    Profiler::Get().EndSession();
    std::cout << "Location: " << location << std::endl;
    std::cout << "Location2: " << location2 << std::endl;
    std::cout << "Location3: " << location3 << std::endl;
    std::cout << "Location4: " << found << std::endl;
    std::cout << "Found2: " << found2 << std::endl;
    return 0;
}

std::string loadFile(std::string filename){
    std::ifstream t(filename);
    std::stringstream buffer;
    buffer << t.rdbuf();
    return (buffer.str());
}

int stringSearchASM(const char* haystack, const char* needle) {
    // Special case: empty needle matches at beginning.
    if (!needle[0])
        return 0;

    int needle_len   = (int)strlen(needle);
    int haystack_len = (int)strlen(haystack);
    if (needle_len > haystack_len)
        return -1;

    // If the needle is a single character, use memchr.
    if (needle_len == 1) {
        const char* found = (const char*)memchr(haystack, needle[0], haystack_len);
        return found ? (int)(found - haystack) : -1;
    }

    // Use AVX2 to process 32 bytes at a time.
    const char firstChar = needle[0];
    __m256i firstVec   = _mm256_set1_epi8(firstChar);
    int limit          = haystack_len - needle_len;
    int i              = 0;

    // Process blocks of 32 bytes.
    for (; i <= limit - 31; i += 32) {
        // Load 32 bytes from haystack (unaligned load).
        __m256i block = _mm256_loadu_si256((const __m256i*)(haystack + i));
        // Compare each byte with the needle’s first character.
        __m256i cmp = _mm256_cmpeq_epi8(block, firstVec);
        // Generate a bitmask: each bit represents a byte match.
        int mask = _mm256_movemask_epi8(cmp);
        while (mask) {
            // Get index of the least significant set bit.
            int bitIndex = __builtin_ctz(mask);
            int pos = i + bitIndex;
            // Verify that the full needle matches.
            if (memcmp(haystack + pos, needle, needle_len) == 0)
                return pos;
            // Clear the lowest set bit.
            mask &= mask - 1;
        }
    }

    // Process any remaining bytes.
    for (; i <= limit; i++) {
        if (haystack[i] == firstChar && memcmp(haystack + i, needle, needle_len) == 0)
            return i;
    }

    return -1;
}
