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
  qs_molecule_print(molecule);

  printf("========= SMILES formula for Cyclohexane ===========\n");

  const char *smiles2 = "C1CCCCC1";
  qs_Tokenizer t2 = {0};
  qs_tokenize(&a, &t2, smiles2);
  qs_ASTNode *root2 = qs_parse_tokens(&a, t2.tokens, t2.token_count);
  qs_print_ast(root2, 0);

  qs_Molecule *molecule2 = qs_molecule_create(&a, root2);
  qs_molecule_print(molecule2);

  qs_arena_destroy(&a);

  return 0;
}
