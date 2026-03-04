# quick-smiles

A single-header SMILES parser in C. Tokenizes a SMILES string and builds a syntax tree.

This is an stb-style header-only library. Define `QUICK_SMILES_IMPLEMENTATION` in exactly one source file before including the header.

## Usage

```c
#define QUICK_SMILES_IMPLEMENTATION
#include "quick-smiles.h"

int main() {
  Arena a = arena_create();

  const char *smiles = "[C:23]1[C:2](-C-C-C)CC#C(=CO)C1";
  Tokenizer t = {0};
  tokenize(&a, &t, smiles);

  ASTNode *root = parse_tokens(&a, t.tokens, t.token_count);

  char buf[64];
  print_ast(root, buf, 0);

  arena_destroy(&a);
  return 0;
}
```

## Building the example

```
make
./out/example
```

## AST structure

The parser produces a tree with four node types:

- **Atom** -- leaf node holding the element symbol and optional atom-map label (e.g. `C:23`).
- **Bond** -- holds the bond order (1 = single, 2 = double, 3 = triple). Appears explicitly between atoms in a chain.
- **Chain** -- ordered sequence of alternating atoms and bonds.
- **Branch** -- represents a `(...)` group. Starts with a bond node indicating how the branch connects to its parent atom, followed by a sub-chain.

Ring closures are stored as annotations on the atoms that open and close the ring, to be resolved during a later graph-building pass.

For the input `[C:23]1[C:2](-C-C-C)CC#C(=CO)C1`, the output looks like:

```
AST_CHAIN
  AST_ATOM: C:23
    AST_RING_BOND
  AST_BOND: (1)
  AST_ATOM: C:2
    AST_BRANCH
      AST_BOND: (1)
      AST_CHAIN
        AST_ATOM: C
        AST_BOND: (1)
        AST_ATOM: C
        AST_BOND: (1)
        AST_ATOM: C
  AST_ATOM: C
  AST_BOND: (1)
  AST_ATOM: C
  AST_BOND: (3)
  AST_ATOM: C
    AST_BRANCH
      AST_BOND: (2)
      AST_CHAIN
        AST_ATOM: C
        AST_BOND: (1)
        AST_ATOM: O
  AST_ATOM: C
    AST_RING_BOND
```

## License

MIT
