# Author: Christopher Dubbs
# Date: 2/23/18
# Description: Makefile for smallsh program

# C Compiler Name
CC = gcc

# Flags
CFLAGS = -Wall

# Linker Flags
#LDFLAGS

# Object files (.o files)
OBJS = smallsh.o dynArr.o

# Source files (.c files)
SRCS = smallsh.c dynArr.c

# Header files (.h files)
HEADERS = smallsh.h dynArr.h 

smallsh: ${SRCS} ${HEADERS} 
	${CC} ${CFLAGS} ${SRCS} -o smallsh
clean:
	rm -f *.o