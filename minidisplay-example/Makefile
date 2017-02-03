# Generic Makefile for compiling a simple executable.

LINARO = /home/david/Ingenieria/herramientas/compiladores/gcc-linaro-arm-linux-gnueabihf
XCC := ${LINARO}/bin/arm-linux-gnueabihf-gcc
SRCDIR := src
BUILDDIR := build
INC = -I$(LINARO)/arm-linux-gnueabihf/libc/usr/include -I/usr/include
CFLAGS := -g -Wall -lm
TARGET := minidisplay-test

SRCEXT := c
SOURCES := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o))
DEPS := $(OBJECTS:.o=.deps)

$(TARGET): $(OBJECTS)
	@echo " Linking..."; $(XCC) $^ $(INC) -o $(TARGET)

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(BUILDDIR)
	@echo " CC $<"; $(XCC) $(INC) $(CFLAGS) -MD -MF $(@:.o=.deps) -c -o $@ $<

clean:
	@echo " Cleaning..."; $(RM) -r $(BUILDDIR) $(TARGET)

-include $(DEPS)

.PHONY: clean
