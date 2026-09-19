CC = gcc
WINDRES = windres

TARGET = dat.exe

CFILES = $(wildcard *.c)
OFILES = $(CFILES:.c=.o)

RESOURCE = resource.o

LIBS = -lcrypto

$(TARGET): $(OFILES) $(RESOURCE)
	$(CC) $(OFILES) $(RESOURCE) -o $(TARGET) $(LIBS)

%.o: %.c
	$(CC) -c $< -o $@

$(RESOURCE): resource.rc
	$(WINDRES) resource.rc -o resource.o

clean:
	rm -f $(OFILES) $(RESOURCE)