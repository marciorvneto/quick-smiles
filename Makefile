OUT_DIR := ./out
EXAMPLES_DIR := ./examples

$(OUT_DIR)/basic: $(EXAMPLES_DIR)/basic.c quick-smiles.h | $(OUT_DIR)
	gcc -o $(OUT_DIR)/basic $(EXAMPLES_DIR)/basic.c -g -lm -I.

$(OUT_DIR)/stereochem: $(EXAMPLES_DIR)/stereochem.c quick-smiles.h | $(OUT_DIR)
	gcc -o $(OUT_DIR)/stereochem $(EXAMPLES_DIR)/stereochem.c -g -lm -I.

$(OUT_DIR)/molecule: $(EXAMPLES_DIR)/molecule.c quick-smiles.h | $(OUT_DIR)
	gcc -o $(OUT_DIR)/molecule $(EXAMPLES_DIR)/molecule.c -g -lm -I.

$(OUT_DIR):
	@mkdir -p $(OUT_DIR)


all: $(OUT_DIR)/basic $(OUT_DIR)/stereochem $(OUT_DIR)/molecule


clean:
	rm -rf $(OUT_DIR)

