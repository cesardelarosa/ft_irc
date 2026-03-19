#include "Utils.hpp"

/**
 * @brief Converts a string to IRC lowercase per RFC 1459.
 * @details In addition to standard A-Z -> a-z, the following mappings apply:
 *          [ -> {, ] -> }, \\ -> |, ~ -> ^
 * @param str The string to convert.
 * @return The lowercase version using IRC rules.
 */
std::string toIrcLower(const std::string &str) {
	std::string result = str;
	for (size_t i = 0; i < result.length(); ++i) {
		if (result[i] >= 'A' && result[i] <= 'Z') {
			result[i] += 32;
		} else if (result[i] == '[') {
			result[i] = '{';
		} else if (result[i] == ']') {
			result[i] = '}';
		} else if (result[i] == '\\') {
			result[i] = '|';
		} else if (result[i] == '~') {
			result[i] = '^';
		}
	}
	return result;
}

/**
 * @brief Converts a string to uppercase (standard ASCII).
 * @param str The string to convert.
 * @return The uppercase version of the string.
 */
std::string toUpper(const std::string &str) {
	std::string result = str;
	for (size_t i = 0; i < result.length(); ++i) {
		if (result[i] >= 'a' && result[i] <= 'z')
			result[i] -= 32;
	}
	return result;
}
