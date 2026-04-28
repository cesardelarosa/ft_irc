#include "Bot.hpp"
#include "Channel.hpp"
#include "Constants.hpp"
#include "Server.hpp"
#include "Utils.hpp"
#include <ctime>

// ═══════════════════════════════════════════════════════════════
//  Internal Bot
// ═══════════════════════════════════════════════════════════════

Bot::Bot(Server *server) : _server(server) {
}

Bot::~Bot() {
}

bool Bot::isNickname(const std::string &nick) const {
	return toIrcLower(nick) == "bot";
}

void Bot::handlePrivmsg(const std::string &target, const std::string &text) {
	if (text.empty() || text[0] != '!')
		return;

	std::string reply = _getReply(text);
	if (reply.empty())
		return;

	_sendNotice(target, reply);
}

std::string Bot::_getReply(const std::string &text) const {
	if (text == "!help") {
		return "commands: !help !ping !time";
	}
	if (text == "!ping") {
		return "pong";
	}
	if (text == "!time") {
		return "time " + _getTime();
	}
	return "unknown command";
}

std::string Bot::_getTime() const {
	std::time_t now = std::time(NULL);
	std::tm    *local_time = std::localtime(&now);
	char        buffer[20];

	if (local_time == NULL ||
	    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S",
	                  local_time) == 0) {
		return "unavailable";
	}
	return std::string(buffer);
}

void Bot::_sendNotice(const std::string &target, const std::string &text) {
	std::string msg = ":" + IRC::Identity::BOT_PREFIX + " NOTICE " + target + " :" + text;

	if (!target.empty() && (target[0] == '#' || target[0] == '&')) {
		Channel *channel = _server->getChannel(target);
		if (channel != NULL) {
			channel->broadcastMessage(msg, NULL);
		}
	} else {
		Client *target_client = _server->getClientByNickname(target);
		if (target_client != NULL) {
			_server->sendToClient(*target_client, msg + "\r\n");
		}
	}
}
