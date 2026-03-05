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
