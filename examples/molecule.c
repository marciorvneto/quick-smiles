#define QUICK_SMILES_IMPLEMENTATION
#include "quick-smiles.h"

int main() {
  qs_Arena a = qs_arena_create();

  printf("========= SMILES formula for L-Threonine ===========\n");

  const char *smiles = "C[C@H]([C@@H](C(=O)O)N)O";
  qs_Tokenizer t = {0};
  qs_tokenize(&a, &t, smiles);
  qs_ASTNode *root = qs_parse_tokens(&a, t.tokens, t.token_count);
  qs_print_ast(root, 0);

  qs_Molecule *molecule = qs_molecule_create(&a, root);

  qs_arena_destroy(&a);

  return 0;
}
