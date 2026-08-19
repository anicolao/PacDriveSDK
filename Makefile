CC ?= cc
PKG_CONFIG ?= pkg-config
HIDAPI_PKG ?= $(shell for package in hidapi-hidraw hidapi-libusb hidapi; do \
	if $(PKG_CONFIG) --exists $$package; then echo $$package; break; fi; \
	done)

ifeq ($(strip $(HIDAPI_PKG)),)
$(error HIDAPI was not found by pkg-config; install hidapi and pkg-config)
endif

CPPFLAGS += $(shell $(PKG_CONFIG) --cflags $(HIDAPI_PKG))
CFLAGS ?= -O2
CFLAGS += -std=c11 -Wall -Wextra -Wpedantic
LDLIBS += $(shell $(PKG_CONFIG) --libs $(HIDAPI_PKG))

TARGET = build/usbbutton_tool
CHECK_CONFIG = build/ctrl-w.ubn
VENDOR_CONFIG = build/vendor-example.ubn

all: $(TARGET)

$(TARGET): usbbutton_tool.c
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< $(LDFLAGS) $(LDLIBS)

check: $(TARGET)
	$(TARGET) --configure --dry-run --permanent --mode extended \
		--primary ctrl+w --output $(CHECK_CONFIG)
	test "$$(od -An -tx1 -j10 -N6 $(CHECK_CONFIG) | tr -d ' \n')" = 701a00000000
	$(TARGET) --configure --dry-run --permanent --mode extended \
		--primary 'win+r;w,w,w,period,u,s;b,b,u,t,t,o;n,period,c,o,m,enter' \
		--output $(VENDOR_CONFIG)
	test "$$(od -An -tx1 -j10 -N24 $(VENDOR_CONFIG) | tr -d ' \n')" = \
		7315000000001a1a1a371816050518171712113706121028

clean:
	rm -f $(TARGET) $(CHECK_CONFIG) $(VENDOR_CONFIG)

.PHONY: all check clean
