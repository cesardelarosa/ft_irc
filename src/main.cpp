#include "Server.hpp"
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>

volatile sig_atomic_t g_shutdown = 0;

/**
 * @brief Signal handler for SIGINT and SIGTERM.
 * @details Sets the global shutdown flag for a clean exit from the event loop.
 */
static void signalHandler(int signum) {
	(void)signum;
	g_shutdown = 1;
}

/**
 * @brief Configures signal handling for the server process.
 * @details Uses sigaction to handle SIGINT/SIGTERM (clean shutdown)
 *          and to ignore SIGPIPE and SIGQUIT (prevents crash on send to
 *          closed socket or Ctrl+\ core dump).
 */
static void setupSignals() {
	struct sigaction sa;

	std::memset(&sa, 0, sizeof(sa));
	sa.sa_handler = signalHandler;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	struct sigaction sa_ignore;

	std::memset(&sa_ignore, 0, sizeof(sa_ignore));
	sa_ignore.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa_ignore, NULL);
	sigaction(SIGQUIT, &sa_ignore, NULL);
}

/**
 * @brief The main entry point for the IRC server executable.
 * @details Parses command-line arguments, validates them, sets up signal
 * handling, and starts the server.
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

	setupSignals();

	try {
		Server server(port, password);
		server.start();
	} catch (const std::exception &e) {
		std::cerr << "Server failed to start: " << e.what() << std::endl;
		return (EXIT_FAILURE);
	}

	return (EXIT_SUCCESS);
}
