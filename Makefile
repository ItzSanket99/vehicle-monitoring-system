CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -O2
BIN_DIR = build

.PHONY: all clean

all: $(BIN_DIR)/vehicle_ecu $(BIN_DIR)/dashboard_ecu

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/vehicle_ecu: src/vehicle_ecu.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $<

$(BIN_DIR)/dashboard_ecu: src/dashboard_ecu.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf $(BIN_DIR)
