# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Project Cerium

V ?= 0
ifeq ($(V),0)
  Q = @
  ECHO = @echo
else
  Q =
  ECHO = @true
endif

CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -static -pthread -I./include
LDFLAGS ?= -pthread

SRC_DIR := src
INC_DIR := include
OBJ_DIR := obj

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

TARGET := cbench

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(ECHO) "  LD      $@"
	$(Q)$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(ECHO) "  CC      $<"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	$(Q)mkdir -p $(OBJ_DIR)

clean:
	$(ECHO) "  CLEAN   $(OBJ_DIR) $(TARGET)"
	$(Q)rm -rf $(OBJ_DIR) $(TARGET)
