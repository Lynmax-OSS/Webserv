NAME= webserv

CXX=c++

CFLAG= -Wall -Werror -Wextra -std=c++98

SRCS= src/main.cpp src/ConfigSrc/ConfigParser.cpp src/ConfigSrc/ConfigValidator.cpp \
	  src/ConfigSrc/Tokenizer.cpp src/NetworkSrc/PollManager.cpp src/NetworkSrc/ServerSocket.cpp\
	  src/NetworkSrc/SocketManager.cpp

OBJS=$(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CFLAG) -o $@ $^

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re