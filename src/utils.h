#pragma once
#include <string>
#include <vector>

namespace DEBAR {

/**
 * @brief Utility class providing various helper functions.
 */

class Utils {

public:
    /**
     * @brief Download file from url.
     * @param url The url of file.
     * @param path The path to save file.
     * @return true if download file successfully.
     */
    static bool download_file(const std::string& url, const std::string& path, const char* prefix);

    /**
     * @brief Formats a given size in bytes into a human-readable string.
     * 
     * This function takes a size in bytes and converts it into a more
     * readable format, such as KB, MB, GB, etc.
     * 
     * @param size The size in bytes to be formatted.
     * @return A string representing the formatted size.
     */
    static std::string format_size(size_t size);

    /**
     * @brief Split a string by a delimiter.
     * @param str The string to be split.
     * @param split The delimiter used to split the string.
     * @return A vector of substrings obtained by splitting the input string.
     */
    static std::vector<std::string> split_str(const std::string& str, const std::string& split);
};
}
