#define QUICK_SMILES_IMPLEMENTATION
#include "quick-smiles.h"

int main() {
  qs_Arena a = qs_arena_create();

  printf("========= SMILES formula for L-Threonine ===========\n");

  const char *smiles = "C[C@H]([C@@H](C(=O)O)N)O";
  printf("%s\n", smiles);

  printf("========= Tokens ===========\n");

  qs_Tokenizer t = {0};
  qs_tokenize(&a, &t, smiles);
  qs_print_tokens(t.tokens, t.token_count);

  printf("========= AST ===========\n");

  qs_ASTNode *root = qs_parse_tokens(&a, t.tokens, t.token_count);
  qs_print_ast(root, 0);

  printf("\n========= SMILES formula for trans-2-butene ===========\n");

  const char *smiles2 = "C/C=C/C";
  printf("%s\n", smiles2);

  printf("========= Tokens ===========\n");

  qs_Tokenizer t2 = {0};
  qs_tokenize(&a, &t2, smiles2);
  qs_print_tokens(t2.tokens, t2.token_count);

  printf("========= AST ===========\n");

  qs_ASTNode *root2 = qs_parse_tokens(&a, t2.tokens, t2.token_count);
  qs_print_ast(root2, 0);

  qs_arena_destroy(&a);

  return 0;
}
