CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98
INC = -Iinclude

NAME = webserv

SRCS = src/main.cpp \
       src/ParserSrc/HttpParser.cpp \
	   src/ParserSrc/ClientConnection.cpp

# SRCS = src/main.cpp \
#        src/ParserSrc/HttpParser.cpp \
#        src/ConfigSrc/ConfigParser.cpp \
#        src/ConfigSrc/ConfigValidator.cpp \
#        src/ConfigSrc/Tokenizer.cpp \
#        src/NetworkSrc/PollManager.cpp \
#        src/NetworkSrc/ServerSocket.cpp \
#        src/NetworkSrc/SocketManager.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(INC) -o $(NAME) $(OBJS)

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
