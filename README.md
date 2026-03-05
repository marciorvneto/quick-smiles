# quick-smiles

A single-header SMILES parser in C. Tokenizes a SMILES string and builds a syntax tree.

This is an stb-style header-only library. Define `QUICK_SMILES_IMPLEMENTATION` in exactly one source file before including the header.

## Usage

```c
#define QUICK_SMILES_IMPLEMENTATION
#include "quick-smiles.h"

int main() {
  qs_Arena a = qs_arena_create();

  const char *smiles = "Cn1cnc2c1c(=O)n(c(=O)n2C)C";
  qs_Tokenizer t = {0};
  qs_tokenize(&a, &t, smiles);

  qs_ASTNode *root = qs_parse_tokens(&a, t.tokens, t.token_count);
  qs_print_ast(root, 0);

  qs_arena_destroy(&a);
  return 0;
}
```

## Building the examples

```
make all
./out/basic
./out/stereochem
```

## AST structure

The parser produces a tree with five node types:

- **Atom** -- leaf node holding the element symbol, optional chirality (`@`/`@@`), explicit hydrogen count, and atom-map label.
- **Bond** -- holds the bond order and optional direction (`/` or `\` for cis/trans).
- **Chain** -- ordered sequence of alternating atoms and bonds.
- **Branch** -- represents a `(...)` group. Starts with a bond node indicating how the branch connects to its parent atom, followed by a sub-chain.
- **RingBond** -- annotation on an atom marking a ring opening or closure, resolved during graph building.

## License

MIT
