#pragma once

#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>

// ──────────────────────── IRC Case Conversion (RFC 2812) ────────────────────
std::string toIrcLower(const std::string &str);
std::string toUpper(const std::string &str);

// ──────────────────────── ANSI Terminal Colors ─────────────────────────────

// Styles
#define ANSI_RESET "\033[0m"
#define ANSI_BOLD "\033[1m"
#define ANSI_DIM "\033[2m"

// Foreground colors
#define ANSI_BLACK "\033[30m"
#define ANSI_RED "\033[31m"
#define ANSI_GREEN "\033[32m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_BLUE "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN "\033[36m"
#define ANSI_WHITE "\033[37m"

// Bright foreground
#define ANSI_BRIGHT_BLACK "\033[90m"
#define ANSI_BRIGHT_RED "\033[91m"
#define ANSI_BRIGHT_GREEN "\033[92m"
#define ANSI_BRIGHT_YELLOW "\033[93m"
#define ANSI_BRIGHT_BLUE "\033[94m"
#define ANSI_BRIGHT_MAGENTA "\033[95m"
#define ANSI_BRIGHT_CYAN "\033[96m"
#define ANSI_BRIGHT_WHITE "\033[97m"

// Semantic log macros — retro IRC console style
#define LOG_SERVER ANSI_BOLD ANSI_CYAN "[ircserv]" ANSI_RESET " "
#define LOG_CONNECT ANSI_BOLD ANSI_GREEN ">>>" ANSI_RESET " "
#define LOG_DISCONNECT ANSI_BOLD ANSI_RED "<<<" ANSI_RESET " "
#define LOG_CMD ANSI_BRIGHT_YELLOW "-!-" ANSI_RESET " "
#define LOG_ERROR ANSI_BOLD ANSI_RED "[ERROR]" ANSI_RESET " "
#define LOG_WARN ANSI_YELLOW "[WARN]" ANSI_RESET " "
#define LOG_NICK ANSI_BRIGHT_MAGENTA
#define LOG_CHAN ANSI_BRIGHT_CYAN
#define LOG_FD ANSI_DIM
#define LOG_R ANSI_RESET

#endif
