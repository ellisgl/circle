#
# Makefile
#

CIRCLEHOME = ../..
OBJS       = hd44780device.o st7789display.o ili9341display.o ssd1306display.o \
	         chardevice.o ssd1306device.o st7789device.o sh1106/sh1106device.o
EXTRACLEAN = sh1106/*.d sh1106/*.o sh1106/*.a

libdisplay.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
