NAME = ircserv
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -Iinclude -MMD -MP

SRC_DIR = src
OBJ_DIR = obj

SRCS = $(addprefix $(SRC_DIR)/, \
            main.cpp \
	    Server.cpp \
		Socket.cpp \
		EventManager.cpp \
	    Client.cpp \
	    Channel.cpp \
	    CommandHandler.cpp \
	    Bot.cpp \
	    Utils.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

all: $(NAME)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(NAME): $(OBJ_DIR) $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

-include $(OBJS:.o=.d)

.PHONY: all clean fclean re
