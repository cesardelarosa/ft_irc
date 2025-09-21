#include "Server.hpp"
#include <iostream>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>

// Constructor: Initializes server attributes.
Server::Server(int port, std::string password) : _port(port), _password(password), _server_fd(-1) {
    std::cout << "Server created on port: " << this->_port << std::endl;
}

// Destructor: Ensures the server socket is closed upon object destruction.
Server::~Server(void) {
    if (this->_server_fd != -1) {
        close(this->_server_fd);
    }
}

// Orchestrates the server startup by setting up the socket and running the main event loop.
void Server::start(void) {
    _setupServerSocket();
    _runEventLoop();
}

// Handles the creation, configuration, binding, and listening of the server socket.
// Throws a runtime_error if any step fails, ensuring a clean exit.
void Server::_setupServerSocket(void) {
    sockaddr_in address;
    int opt = 1;

    this->_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (this->_server_fd == -1)
        throw std::runtime_error("Failed to create socket.");

    if (setsockopt(this->_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        throw std::runtime_error("Failed to set socket options.");
    
    if (fcntl(this->_server_fd, F_SETFL, O_NONBLOCK) == -1)
        throw std::runtime_error("Failed to set socket to non-blocking.");

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(this->_port);
    if (bind(this->_server_fd, (struct sockaddr *)&address, sizeof(address)) == -1)
        throw std::runtime_error("Failed to bind to port.");

    if (listen(this->_server_fd, 10) == -1)
        throw std::runtime_error("Failed to listen on socket.");

    struct pollfd server_poll_fd;
    server_poll_fd.fd = this->_server_fd;
    server_poll_fd.events = POLLIN;
    this->_fds.push_back(server_poll_fd);
}

// Contains the main server loop that waits for and handles network events.
void Server::_runEventLoop(void) {
    std::cout << "Server is listening on port " << this->_port << "..." << std::endl;

    while (true) {
        if (poll(this->_fds.data(), this->_fds.size(), -1) == -1)
            throw std::runtime_error("poll() failed.");

        if (this->_fds[0].revents & POLLIN)
            _handleNewConnection();

        for (size_t i = 1; i < this->_fds.size(); ++i) {
            if (this->_fds[i].revents & POLLIN)
                _handleClientData(i);
        }
    }
}

// Accepts a new client connection and adds it to the list of file descriptors to monitor.
void Server::_handleNewConnection(void) {
    int client_fd = accept(this->_server_fd, NULL, NULL);
    if (client_fd == -1) {
        std::cerr << "Warning: accept() failed." << std::endl;
        return;
    }

    struct pollfd client_poll_fd;
    client_poll_fd.fd = client_fd;
    client_poll_fd.events = POLLIN;
    this->_fds.push_back(client_poll_fd);

    std::cout << "New connection accepted. Client fd: " << client_fd << std::endl;
}

// Placeholder for handling data received from a client.
void Server::_handleClientData(int client_idx) {
    // This is where the logic to receive (recv) and process messages will go.
    std::cout << "Activity detected on client fd: " << this->_fds[client_idx].fd << std::endl;
    // For now, we do nothing with it.
}