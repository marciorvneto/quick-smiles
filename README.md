# quick-smiles

A single-header SMILES parser and molecule builder in C. Tokenizes a SMILES string, builds a syntax tree, and constructs a molecular graph with atoms, bonds, and ring closures.

This is an stb-style header-only library. Define `QUICK_SMILES_IMPLEMENTATION` in exactly one source file before including the header.

## Usage

```c
#define QUICK_SMILES_IMPLEMENTATION
#include "quick-smiles.h"

int main() {
  qs_Arena a = qs_arena_create();
  const char *smiles = "C[C@H]([C@@H](C(=O)O)N)O";

  qs_Tokenizer t = {0};
  qs_tokenize(&a, &t, smiles);
  qs_ASTNode *root = qs_parse_tokens(&a, t.tokens, t.token_count);

  // Build molecular graph
  qs_Molecule *mol = qs_molecule_create(&a, root);
  qs_molecule_print(mol);

  qs_arena_destroy(&a);
  return 0;
}
```

## Building the examples

```
make all
./out/basic
./out/stereochem
./out/molecule
```

## AST structure

The parser produces a tree with five node types:

- **Atom** -- leaf node holding the element symbol, optional chirality (`@`/`@@`), explicit hydrogen count, and atom-map label.
- **Bond** -- holds the bond order and optional direction (`/` or `\` for cis/trans).
- **Chain** -- ordered sequence of alternating atoms and bonds.
- **Branch** -- represents a `(...)` group. Starts with a bond node indicating how the branch connects to its parent atom, followed by a sub-chain.
- **RingBond** -- annotation on an atom marking a ring opening or closure, resolved during graph building.

## Molecule graph (WIP)

`qs_molecule_create` walks the AST and produces a flat molecular graph. Each `qs_Atom` stores its symbol, chirality, explicit hydrogen count, and an ordered neighbor list. Each `qs_Bond` stores the two atom indices, bond order, and stereo direction. Ring closures from SMILES ring digits are resolved into regular bonds. Neighbor ordering is preserved from the SMILES string for correct chirality interpretation.

## License

MIT
