#define QUICK_SMILES_IMPLEMENTATION
#include "quick-smiles.h"

int main() {
  qs_Arena a = qs_arena_create();

  const char *smiles = "[C:23]1[C:2](-C-C-C)CC#C(=CO)C1";
  qs_Tokenizer t = {0};
  qs_tokenize(&a, &t, smiles);
  qs_ASTNode *root = qs_parse_tokens(&a, t.tokens, t.token_count);

  char buf[64];
  qs_print_ast(root, buf, 0);

  qs_arena_destroy(&a);

  return 0;
}
