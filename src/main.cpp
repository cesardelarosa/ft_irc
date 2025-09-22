#include "Server.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>

int main(int argc, char **argv) {
	if (argc != 3) {
		std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
		return (EXIT_FAILURE);
	}

	int port = atoi(argv[1]);
	if (port <= 0 || port > 65535) {
		std::cerr << "Error: Invalid port number." << std::endl;
		return (EXIT_FAILURE);
	}

	std::string password = argv[2];

	try {
		Server server(port, password);
		server.start();
	} catch (const std::exception &e) {
		std::cerr << "Server failed to start: " << e.what() << std::endl;
		return (EXIT_FAILURE);
	}

	return (0);
}