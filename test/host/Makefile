# Host test harness for ../../src/ide.c -- compiles the REAL driver source with
# the host C compiler against a mock Gayle/ATA model and checks its SCSI->ATA
# translation, probe gating, byte order handling and error paths.
#
#   make check     # build + run; exit 0 == all assertions pass
#   make clean
#
# The driver is K&R C (-std=gnu89).  It is compiled in a kernel-only include
# world (-Istubs shadows sys/types.h, sys/errno.h, sys/inline.h, rico.h, sd.h)
# with HOST_TEST defined and mock_regs.h force-included so its register seam
# lands in the mock.  The harness files are ordinary host C.

CC      ?= cc
CSTD    := -std=gnu89 -fno-builtin
CWARN   := -Wall -Wno-unused-parameter -Wno-unused-variable -Wno-deprecated-non-prototype \
           -Wno-unused-function
CFLAGS  := $(CSTD) $(CWARN) -O0 -g
BUILD   := build
SRC     := ../../src
DRV     := $(SRC)/ide.c

HOST_OBJS := $(BUILD)/mock_gayle.o $(BUILD)/kstubs.o $(BUILD)/ide_test.o
OBJS      := $(BUILD)/ide.o $(HOST_OBJS)
BIN       := $(BUILD)/ide_test

.PHONY: all check clean
all: $(BIN)

$(BUILD)/ide.o: $(DRV) mock_regs.h stubs/rico.h stubs/sd.h \
                stubs/sys/types.h stubs/sys/errno.h stubs/sys/inline.h stubs/sys/param.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -DHOST_TEST -Istubs -I$(SRC) -include mock_regs.h -c $(DRV) -o $@

$(BUILD)/%.o: %.c mock_gayle.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

check: $(BIN)
	./$(BIN)

clean:
	rm -rf $(BUILD)
