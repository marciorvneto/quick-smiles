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
