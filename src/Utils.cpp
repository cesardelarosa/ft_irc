#include "Utils.hpp"

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

std::string toUpper(const std::string &str) {
  std::string result = str;
  for (size_t i = 0; i < result.length(); ++i) {
    if (result[i] >= 'a' && result[i] <= 'z')
      result[i] -= 32;
  }
  return result;
}
