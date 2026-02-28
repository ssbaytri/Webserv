NAME		= webserv

CC			= c++
CFLAGS		= -Wall -Wextra -Werror -std=c++98
RM			= rm -f

# Directories
INC_DIR		= includes
SRC_DIR		= srcs
OBJ_DIR		= objs

# Source files
SRCS		= main.cpp \
			  Server.cpp \
			  Client.cpp \
			  Request.cpp \
			  Response.cpp \
			  Config.cpp   \
			  utils.cpp

# Object files
OBJS		= $(addprefix $(OBJ_DIR)/, $(SRCS:.cpp=.o))

# Header files
HEADERS		= $(INC_DIR)/Server.hpp \
			  $(INC_DIR)/Client.hpp \
			  $(INC_DIR)/Request.hpp \
			  $(INC_DIR)/Response.hpp \
			  $(INC_DIR)/Config.hpp \
			  $(INC_DIR)/utils.hpp

# Colors
GREEN		= \033[0;32m
YELLOW		= \033[0;33m
RED			= \033[0;31m
RESET		= \033[0m

# Rules
all: $(NAME)

$(NAME): $(OBJS)
	@$(CC) $(CFLAGS) $(OBJS) -o $(NAME)
	@echo "$(GREEN)✓ $(NAME) compiled successfully!$(RESET)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	@mkdir -p $(OBJ_DIR)
	@$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@
	@echo "$(YELLOW)Compiling: $<$(RESET)"

clean:
	@$(RM) -r $(OBJ_DIR)
	@echo "$(RED)✗ Object files removed$(RESET)"

fclean: clean
	@$(RM) $(NAME)
	@echo "$(RED)✗ $(NAME) removed$(RESET)"

re: fclean all

.PHONY: all clean fclean re