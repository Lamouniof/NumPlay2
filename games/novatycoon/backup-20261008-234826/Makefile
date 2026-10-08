Q ?= @
CC = arm-none-eabi-gcc
NWLINK ?= npx --yes -- nwlink@1.0.0
BUILD_DIR = output
SRC = NovaTycoon.c
CFLAGS = -std=c11 $(shell $(NWLINK) eadk-cflags-device)
CFLAGS += -Os -Wall -Wextra -Wno-unused-parameter -fno-math-errno
CFLAGS += -fno-tree-loop-distribute-patterns -flto -fno-fat-lto-objects
CFLAGS += -fwhole-program -fvisibility=internal
.PHONY: build module sim-module clean
build:
	@echo "Nova Tycoon utilise les fonctions du lanceur NumPlay."
module: $(BUILD_DIR)/module.o
$(BUILD_DIR)/module.o: $(SRC) | $(BUILD_DIR)
	$(Q) $(CC) $(CFLAGS) -Wl,--relocatable -nostartfiles -nostdlib -Wl,-e,main -flinker-output=nolto-rel $(SRC) -o $@

sim-module: $(BUILD_DIR)/sim-module.o
$(BUILD_DIR)/sim-module.o: $(SRC) | $(BUILD_DIR)
	$(Q) mkdir -p $(BUILD_DIR)/sim
	$(Q) cc -std=c11 -O2 -fPIC $(shell $(NWLINK) eadk-cflags-simulator) -Dmain=np_novatycoon_main -c $(SRC) -o $(BUILD_DIR)/sim/main.o
	$(Q) ld -r $(BUILD_DIR)/sim/main.o -exported_symbol _np_novatycoon_main -o $@

$(BUILD_DIR):
	$(Q) mkdir -p $@

clean:
	$(Q) rm -rf $(BUILD_DIR)
