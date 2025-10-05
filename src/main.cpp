#include "Server.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>

/**
 * @brief The main entry point for the IRC server executable.
 * @details Parses command-line arguments, validates them, and starts the
 * server.
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line argument strings. Expected: <port>
 * <password>.
 * @return Returns EXIT_SUCCESS on successful termination, or EXIT_FAILURE on
 * error.
 */
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

	return (EXIT_SUCCESS);
}