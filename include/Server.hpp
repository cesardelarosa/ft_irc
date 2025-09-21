#pragma once

#ifndef SERVER_HPP
# define SERVER_HPP

# include <string>
# include <vector>
# include <poll.h>

class Server {
    public:
        // Constructor & Destructor
        Server(int port, std::string password);
        ~Server(void);

        // Public method to start the server
        void start(void);

    private:
        // Server attributes
        int                         _port;
        std::string                 _password;
        int                         _server_fd;
        std::vector<struct pollfd>  _fds;

        // Private methods for internal logic
        void _setupServerSocket(void);
        void _runEventLoop(void);
        void _handleNewConnection(void);
        void _handleClientData(int client_idx);
};

#endif