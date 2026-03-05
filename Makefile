OUT_DIR := ./out

$(OUT_DIR)/example: example.c quick-smiles.h | $(OUT_DIR)
	gcc -o $(OUT_DIR)/example example.c -g -lm

$(OUT_DIR):
	@mkdir -p $(OUT_DIR)

clean:
	rm -rf $(OUT_DIR)

