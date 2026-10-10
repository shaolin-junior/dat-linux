CC = gcc
ASM = nasm

TARGET = dat

CFILES = $(wildcard *.c)
ASMFILES = $(wildcard *.asm)

OFILES = $(CFILES:.c=.o) $(ASMFILES:.asm=.o)

LIBS = -lcrypto

$(TARGET): $(OFILES)
	$(CC) $(OFILES) -o $(TARGET) $(LIBS)

%.o: %.c
		$(CC) -Wall -Wextra -c $< -o $@

%.o: %.asm
	$(ASM) -f elf64 $< -o $@

clean:
	rm -f $(OFILES) $(TARGET) $(RESOURCE)