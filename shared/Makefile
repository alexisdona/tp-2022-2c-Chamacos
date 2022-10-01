# -*- MakeFile -*-
CC = gcc
CFLAGS = -Wall -g -c
LINK = -lcommons -lpthread 	
SRC_DIR = src
SRC = $(shell find ./src -type f -iname "*.c" -exec basename {} \;)
OBJ = $(SRC:%.c=./src/obj/%.o)

build: makeobjfolder $(OBJ)
	
./src/obj/%.o: $(SRC_DIR)/%.c
	# @echo "Compiling $<..."
	$(CC) $(CFLAGS) $< -o $@ $(LINK)

makeobjfolder:
	if [ ! -d $(SRC_DIR)/obj ];\
		then mkdir $(SRC_DIR)/obj;\
	fi

run: build
	./$(OUTFILE)

.PHONY: clean

clean:
	if [ -d $(SRC_DIR)/obj ];\
		then rm -rf $(SRC_DIR)/obj;\
	fi
	if [ -f log.log ];\
		then rm log.log;\
	fi
