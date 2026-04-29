#include "Server.hpp"
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>

volatile sig_atomic_t g_shutdown = 0;

static void signalHandler(int signum) {
  (void)signum;
  g_shutdown = 1;
}

static void setupSignals() {
  struct sigaction sa;

  std::memset(&sa, 0, sizeof(sa));
  sa.sa_handler = signalHandler;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  // Ignore SIGPIPE to prevent crashing when writing to a closed socket
  // Ignore SIGQUIT so Ctrl+\ doesn't kill the server
  struct sigaction sa_ignore;

  std::memset(&sa_ignore, 0, sizeof(sa_ignore));
  sa_ignore.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &sa_ignore, NULL);
  sigaction(SIGQUIT, &sa_ignore, NULL);
}

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
