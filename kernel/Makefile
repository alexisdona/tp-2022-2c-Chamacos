# -*- MakeFile -*-
CC = gcc
CFLAGS = -Wall -g -c
LINK = -lcommons -lpthread
OUTFILE = bin/mod.out
SRC_DIR = src
SRC = $(shell find ./src -type f -iname "*.c" -exec basename {} \;)
OBJ = $(SRC:%.c=./src/obj/%.o)
SHARED = $(shell find ./../shared/src/obj/*.o)

build: makeshared makefolders $(OBJ) $(SHARED)
	@echo "Building..."
	$(CC) -o $(OUTFILE) $(OBJ) $(SHARED) $(LINK)
	@echo "done."

./src/obj/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $< -o $@ $(LINK)

makeshared:
	@echo "Compiling shared..."
	make -C ../shared

makefolders:
	if [ ! -d $(SRC_DIR)/../bin ];\
		then mkdir $(SRC_DIR)/../bin;\
	fi
	if [ ! -d $(SRC_DIR)/obj ];\
		then mkdir $(SRC_DIR)/obj;\
	fi

run: build
	./$(OUTFILE)

.PHONY: clean

clean:
	if [ -d $(SRC_DIR)/../bin ];\
		then rm -rf $(SRC_DIR)/../bin;\
	fi
	if [ -d $(SRC_DIR)/obj ];\
		then rm -rf $(SRC_DIR)/obj;\
	fi
	if [ -f $(OUTFILE) ];\
		then rm $(OUTFILE);\
	fi
	if [ -f log.log ];\
		then rm log.log;\
	fi
