# quick-smiles

A single-header SMILES parser in C. Tokenizes a SMILES string and builds a syntax tree.

This is an stb-style header-only library. Define `QUICK_SMILES_IMPLEMENTATION` in exactly one source file before including the header.

## Usage

```c
#define QUICK_SMILES_IMPLEMENTATION
#include "quick-smiles.h"

int main() {
  qs_Arena a = qs_arena_create();

  printf("========= SMILES formula for Caffeine ===========\n");

  const char *smiles = "Cn1cnc2c1c(=O)n(c(=O)n2C)C";
  printf("%s\n", smiles);

  printf("========= Tokens ===========\n");

  qs_Tokenizer t = {0};
  qs_tokenize(&a, &t, smiles);
  qs_print_tokens(t.tokens, t.token_count);

  printf("========= AST ===========\n");

  qs_ASTNode *root = qs_parse_tokens(&a, t.tokens, t.token_count);
  qs_print_ast(root, 0);

  qs_arena_destroy(&a);

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
QS_AST_CHAIN
  QS_AST_ATOM: C
  QS_AST_BOND: (1)
  QS_AST_ATOM: n
    QS_AST_RING_BOND: (1)
  QS_AST_BOND: (1)
  QS_AST_ATOM: c
  QS_AST_BOND: (1)
  QS_AST_ATOM: n
  QS_AST_BOND: (1)
  QS_AST_ATOM: c
    QS_AST_RING_BOND: (2)
  QS_AST_BOND: (1)
  QS_AST_ATOM: c
    QS_AST_RING_BOND: (1)
  QS_AST_BOND: (1)
  QS_AST_ATOM: c
    QS_AST_BRANCH
      QS_AST_BOND: (2)
      QS_AST_CHAIN
        QS_AST_ATOM: O
  QS_AST_ATOM: n
    QS_AST_BRANCH
      QS_AST_BOND: (1)
      QS_AST_CHAIN
        QS_AST_ATOM: c
          QS_AST_BRANCH
            QS_AST_BOND: (2)
            QS_AST_CHAIN
              QS_AST_ATOM: O
        QS_AST_ATOM: n
          QS_AST_RING_BOND: (2)
        QS_AST_BOND: (1)
        QS_AST_ATOM: C
  QS_AST_ATOM: C
```

## License

MIT
