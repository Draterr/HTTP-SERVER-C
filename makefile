server:
	gcc -o bin/server $(shell find src/ -name "*.c") -lz 
