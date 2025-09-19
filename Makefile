CC=gcc
CFLAGS=-I/usr/include/hidapi -Wall
LIBS=-lhidapi-libusb

TARGET=usbbutton_tool

all: $(TARGET)

$(TARGET): usbbutton_tool.c
	$(CC) $(CFLAGS) -o $(TARGET) usbbutton_tool.c $(LIBS)

clean:
	rm -f $(TARGET)
