#pragma once

#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <cstddef>
#include <ctime>
#include <string>

/**
 * @brief Central repository for cross-cutting configuration constants.
 * @details Values used by multiple classes or that define server-wide policy
 *          live here. Class-local magic numbers stay as private constants in
 *          their own class.
 */
namespace IRC {

// ──────────────────────── Server Identity ────────────────────────
namespace Identity {
	const std::string SERVER_NAME   = "ircserv";
	const std::string SERVER_INFO   = "ft_irc server";
	const std::string BOT_PREFIX    = "Bot!bot@" + SERVER_NAME;
}

// ──────────────────────── Buffer Limits ─────────────────────────
namespace Limits {
	const size_t RECV_BUFFER_MAX    = 4096;   // Max receive buffer before drop
	const size_t SEND_BUFFER_MAX    = 65536;  // Max send queue before SendQ kill
	const size_t MAX_IRC_LINE       = 510;    // RFC 2812 max command length
	const int    LISTEN_BACKLOG     = 10;     // listen() backlog queue
	const size_t MAX_NICK_LENGTH    = 9;      // RFC 2812 max nickname length
}

// ──────────────────────── Network / Timing ──────────────────────
namespace Timing {
	const int    POLL_TIMEOUT_MS    = 5000;   // poll() wait (milliseconds)
	const int    TIMEOUT_CHECK_INTERVAL = 10; // Seconds between timeout scans
	const time_t PING_IDLE_START    = 120;    // Seconds idle before PING
	const time_t PING_IDLE_END      = 130;    // Upper bound of PING window
	const time_t TIMEOUT_DISCONNECT = 180;    // Seconds idle before disconnect
}

} // namespace IRC

#endif
