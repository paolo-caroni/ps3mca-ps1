CC ?= gcc
CFLAGS ?= $(shell pkg-config --cflags libusb-1.0)
LDFLAGS ?= $(shell pkg-config --libs libusb-1.0)

ps3mca-ps1:
	$(CC) $(CFLAGS) $(LDFLAGS) src/main.c -o ps3mca-ps1
	$(CC) $(CFLAGS) $(LDFLAGS) -D DEBUG src/main.c -o ps3mca-ps1-debug

.PHONY: clean
clean:
	rm -f ps3mca-ps1
	rm -f ps3mca-ps1-debug
