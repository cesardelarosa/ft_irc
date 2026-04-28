*This project has been created as part of the 42 curriculum by cde-la-r, adpedrer, acaro-su.*

## Description
ft_irc is a custom Internet Relay Chat (IRC) server developed entirely in C++98. The primary goal of this project is to understand network programming, socket multiplexing, and client-server communication architectures. 

The server is designed to be highly robust, handling multiple concurrent connections without hanging or crashing. It strictly utilizes non-blocking I/O operations and relies on a single poll() instance to monitor all network events, ensuring efficient resource management without the use of threading or forking.

The project implements the core mechanics of the IRC protocol, allowing users to authenticate, join channels, exchange private and group messages, and manage channel privileges. It fully supports standard operator commands such as KICK, INVITE, TOPIC, and MODE, making the server compatible with official IRC reference clients.

## Instructions
### Compilation
A Makefile is provided to compile the source code. To build the executable, simply run:

    make

This will compile the project using the required flags (-Wall -Wextra -Werror -std=c++98) and generate the `ircserv` binary.

### Execution
The server requires a listening port and a connection password to start. Run the executable as follows:

    ./ircserv <port> <password>

Example:
    ./ircserv 6667 pass123

### Connecting and Testing
You can connect to the server using any standard IRC client (such as irssi, WeeChat, or HexChat) by pointing it to `127.0.0.1` on the specified port, using the provided password.

Alternatively, you can test raw protocol commands using netcat:

    nc -C 127.0.0.1 <port>

## Resources
The development of this project relied on the following primary resources:
- [RFC 2812 (Internet Relay Chat: Client Protocol)](https://www.rfc-editor.org/rfc/rfc2812): The definitive standard used to implement command parsing, numerical replies, and network behavior.
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/html/): Used as a primary reference for understanding POSIX sockets, non-blocking I/O, and the poll() system call.
- [The C++98 Standard Documentation (cppreference)](https://en.cppreference.com/w/cpp/): For ensuring standard compliance and avoiding forbidden external libraries.

### AI Usage
In accordance with the project guidelines, Artificial Intelligence tools were utilized for the following tasks:
- Assisting in the interpretation and summarization of specific sections of RFC 2812.
- Generating test ideas and stress-testing edge cases for network buffer aggregation.
- Drafting and structuring the initial version of this README file.
- Exploring alternative architectural approaches during the initial design phase before committing to the final implementation.
