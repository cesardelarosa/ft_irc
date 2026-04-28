#pragma once

#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <cstddef>
#include <ctime>
#include <string>

namespace IRC {

namespace Identity {
const std::string SERVER_NAME = "ircserv";
const std::string SERVER_INFO = "ft_irc server";
const std::string BOT_PREFIX = "Bot!bot@" + SERVER_NAME;
} // namespace Identity

namespace Limits {
const size_t RECV_BUFFER_MAX = 4096;
const size_t SEND_BUFFER_MAX = 65536;
const size_t MAX_IRC_LINE = 510;
const int LISTEN_BACKLOG = 10;
const size_t MAX_NICK_LENGTH = 9;
} // namespace Limits

namespace Timing {
const int POLL_TIMEOUT_MS = 5000;
const int TIMEOUT_CHECK_INTERVAL = 10;
const time_t PING_IDLE_START = 120;
const time_t PING_IDLE_END = 130;
const time_t TIMEOUT_DISCONNECT = 180;
} // namespace Timing

} // namespace IRC

#endif
