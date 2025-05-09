#include <string>

/**
 * @brief Searches for all occurrences of a substring in a string using an OpenCL kernel.
 *
 * This function sets up an OpenCL context and queue, builds a simple substring-search
 * kernel, dispatches it over each possible start position, and collects up to 100 match indices.
 * Profiling scopes (via PROFILE_FUNCTION and PROFILE_SCOPE) and timers measure each stage.
 *
 * @param[in] str     The input text to search in.
 * @param[in] substr  The pattern to search for.
 *
 * @return A vector of starting indices where `substr` was found in `str`. Contains at most
 *         100 entries, in ascending order.
 *
 * @throws cl::Error if any OpenCL call fails (e.g., context creation, program build, buffer
 *         operations, or kernel enqueue). Build failures will also print the build log
 *         to stderr before rethrowing.
 */
std::vector<int> clSearch(const std::string& str,const std::string& substr);
