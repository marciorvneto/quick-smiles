OUT_DIR := ./out
EXAMPLES_DIR := ./examples

SRCS := $(wildcard $(EXAMPLES_DIR)/*.c)
BINS := $(patsubst $(EXAMPLES_DIR)/%.c, $(OUT_DIR)/%, $(SRCS))

.PHONY: all clean

all: $(BINS)

$(OUT_DIR)/%: $(EXAMPLES_DIR)/%.c quick-smiles.h | $(OUT_DIR)
	gcc -o $@ $< -g -lm -I. -Wall -Werror

$(OUT_DIR):
	@mkdir -p $(OUT_DIR)

clean:
	rm -rf $(OUT_DIR)

