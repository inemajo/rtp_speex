NAME	=	svc

SRC	=	main.o \
		options.o \
		rtp.o \
		speex.o

LIBS	=	-lspeex -lortp

# Use std=c99 and no ansi because ortp lib haven't c89 (iso9899:1990)
# compatibility
CFLAGS	=	-Wall -Werror -pedantic -std=c99 -g

all:	$(SRC)

	$(CC) $(LIBS) $(CFLAGS) $(SRC) -o $(NAME)

clean:
	rm -f $(SRC)

fclean: clean
	rm -f $(NAME)

re: fclean all
