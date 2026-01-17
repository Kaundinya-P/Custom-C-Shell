CC = gcc
CFLAGS = -std=c99 \
         -D_POSIX_C_SOURCE=200809L \
         -D_XOPEN_SOURCE=700 \
         -Wall -Wextra -Werror \
         -Wno-unused-parameter \
         -fno-asm \
         -Iinclude

SRC_DIR = src
INC_DIR = include
OBJ = shell_prompt.o parser.o hop.o reveal.o command_execution.o log.o redirection.o sequential_execution.o background_execution.o activities.o ping.o job_control.o signals.o
TARGET = shell.out

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

shell_prompt.o: $(SRC_DIR)/shell_prompt.c $(INC_DIR)/parser.h $(INC_DIR)/hop.h $(INC_DIR)/reveal.h $(INC_DIR)/command_execution.h $(INC_DIR)/log.h $(INC_DIR)/redirection.h $(INC_DIR)/sequential_execution.h $(INC_DIR)/background_execution.h $(INC_DIR)/activities.h $(INC_DIR)/ping.h $(INC_DIR)/job_control.h $(INC_DIR)/signals.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/shell_prompt.c

parser.o: $(SRC_DIR)/parser.c $(INC_DIR)/parser.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/parser.c

hop.o: $(SRC_DIR)/hop.c $(INC_DIR)/hop.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/hop.c

reveal.o: $(SRC_DIR)/reveal.c $(INC_DIR)/reveal.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/reveal.c

command_execution.o: $(SRC_DIR)/command_execution.c $(INC_DIR)/command_execution.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/command_execution.c

log.o: $(SRC_DIR)/log.c $(INC_DIR)/log.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/log.c

redirection.o: $(SRC_DIR)/redirection.c $(INC_DIR)/redirection.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/redirection.c

sequential_execution.o: $(SRC_DIR)/sequential_execution.c $(INC_DIR)/sequential_execution.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/sequential_execution.c

background_execution.o: $(SRC_DIR)/background_execution.c $(INC_DIR)/background_execution.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/background_execution.c

activities.o: $(SRC_DIR)/activities.c $(INC_DIR)/activities.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/activities.c

ping.o: $(SRC_DIR)/ping.c $(INC_DIR)/ping.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/ping.c

job_control.o: $(SRC_DIR)/job_control.c $(INC_DIR)/job_control.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/job_control.c

signals.o: $(SRC_DIR)/signals.c $(INC_DIR)/signals.h $(INC_DIR)/job_control.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/signals.c

clean:
	rm -f $(OBJ) $(TARGET)
