#include "CommandHandler.hpp"
#include "Server.hpp"

CommandHandler::CommandHandler(Server *server) : _server(server) {
}

CommandHandler::~CommandHandler() {
}

void CommandHandler::handleCommand(Client &client, std::string const &message) {
	(void)client;
	(void)message;
}
