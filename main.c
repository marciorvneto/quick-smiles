#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//===============================
//
//   Arena
//
//===============================

#define MAX_ARENA_SIZE 1024 * 1024 // 1MB
typedef struct {
  void *base;
  size_t offset;
  size_t capacity;
} Arena;

Arena arena_create() {
  Arena a;
  a.base = malloc(MAX_ARENA_SIZE);
  a.offset = 0;
  a.capacity = MAX_ARENA_SIZE;
  return a;
}

void *arena_alloc(Arena *a, size_t size) {
  size_t amount = (size + 7) & ~7;
  if (a->offset + amount < a->capacity) {
    void *addr = a->base + a->offset;
    a->offset += amount;
    return addr;
  } else {
    printf("Arena ran out of memory\n");
    return NULL;
  }
}

void arena_destroy(Arena *a) { free(a->base); }

//===============================
//
//   Tokenizer
//
//===============================

typedef enum {
  TOKEN_NUMBER,
  TOKEN_ATOM,
  TOKEN_COLON,
  TOKEN_LPAREN,
  TOKEN_RPAREN,
  TOKEN_LBRACKET,
  TOKEN_RBRACKET,
  TOKEN_BOND,

  TOKEN_END,
  TOKEN_ERR,
} token_t;

typedef struct {
  token_t type;
  union {
    union {
      int value;
    } num;
    union {
      const char *atom;
    } atom;
    union {
      int order;
    } bond;
  } as;
} Token;

const char *token_string(Token *tok, char *buf) {
  switch (tok->type) {
  case TOKEN_NUMBER: {
    sprintf(buf, "TOKEN_NUMBER: %d", tok->as.num.value);
    return buf;
  }
  case TOKEN_ATOM: {
    sprintf(buf, "TOKEN_ATOM: %s", tok->as.atom.atom);
    return buf;
  }
  case TOKEN_COLON:
    return "TOKEN_COLON";
  case TOKEN_LPAREN:
    return "TOKEN_LPAREN";
  case TOKEN_RPAREN:
    return "TOKEN_RPAREN";
  case TOKEN_LBRACKET:
    return "TOKEN_LBRACKET";
  case TOKEN_RBRACKET:
    return "TOKEN_RBRACKET";
  case TOKEN_BOND: {
    sprintf(buf, "TOKEN_BOND: %d", tok->as.bond.order);
    return buf;
  }
  case TOKEN_END:
    return "TOKEN_END";
  case TOKEN_ERR:
    return "TOKEN_ERR";
  }
}

#define MAX_TOKENS 1024
typedef struct {
  const char *string;
  size_t pointer;
  Token tokens[MAX_TOKENS];
  size_t token_count;
} Tokenizer;

int parse_int(Tokenizer *tokenizer) {
  int accumulator = 0;
  char current = tokenizer->string[tokenizer->pointer];
  while (isdigit(current)) {
    int digit = current - '0';
    accumulator *= 10;
    accumulator += digit;
    current = tokenizer->string[++tokenizer->pointer];
  }
  return accumulator;
}

const char *parse_atom_str(Arena *a, Tokenizer *tokenizer) {
  char *atom_string = arena_alloc(a, 8);
  char current = tokenizer->string[tokenizer->pointer];
  size_t i = 0;
  while (isalpha(current)) {
    if (i > 0 && isupper(current)) {
      return atom_string;
    }
    atom_string[i] = current;
    i++;
    current = tokenizer->string[++tokenizer->pointer];
  }
  return atom_string;
}

int get_token(Arena *a, Tokenizer *tokenizer, size_t string_length) {
  if (tokenizer->pointer == string_length) {
    return TOKEN_END;
  }
  char current = tokenizer->string[tokenizer->pointer];
  switch (current) {
  case '(': {
    Token tok;
    tok.type = TOKEN_LPAREN;
    tokenizer->tokens[tokenizer->token_count++] = tok;
    tokenizer->pointer++;
    return tok.type;
  }
  case ')': {
    Token tok;
    tok.type = TOKEN_RPAREN;
    tokenizer->tokens[tokenizer->token_count++] = tok;
    tokenizer->pointer++;
    return tok.type;
  }
  case '[': {
    Token tok;
    tok.type = TOKEN_LBRACKET;
    tokenizer->tokens[tokenizer->token_count++] = tok;
    tokenizer->pointer++;
    return tok.type;
  }
  case ']': {
    Token tok;
    tok.type = TOKEN_RBRACKET;
    tokenizer->tokens[tokenizer->token_count++] = tok;
    tokenizer->pointer++;
    return tok.type;
  }
  case ':': {
    Token tok;
    tok.type = TOKEN_COLON;
    tokenizer->tokens[tokenizer->token_count++] = tok;
    tokenizer->pointer++;
    return tok.type;
  }
  case '=': {
    Token tok;
    tok.type = TOKEN_BOND;
    tok.as.bond.order = 2;
    tokenizer->tokens[tokenizer->token_count++] = tok;
    tokenizer->pointer++;
    return tok.type;
  }
  case '#': {
    Token tok;
    tok.type = TOKEN_BOND;
    tok.as.bond.order = 3;
    tokenizer->tokens[tokenizer->token_count++] = tok;
    tokenizer->pointer++;
    return tok.type;
  }
  case '$': {
    Token tok;
    tok.type = TOKEN_BOND;
    tok.as.bond.order = 4;
    tokenizer->tokens[tokenizer->token_count++] = tok;
    tokenizer->pointer++;
    return tok.type;
  }
  default: {
    if (isdigit(current)) {
      int num = parse_int(tokenizer);
      Token tok;
      tok.type = TOKEN_NUMBER;
      tok.as.num.value = num;
      tokenizer->tokens[tokenizer->token_count++] = tok;
      return tok.type;
    }
    if (isalpha(current)) {
      const char *atom = parse_atom_str(a, tokenizer);
      Token tok;
      tok.type = TOKEN_ATOM;
      tok.as.atom.atom = atom;
      tokenizer->tokens[tokenizer->token_count++] = tok;
      return tok.type;
    }
    tokenizer->pointer++;
    return -1;
  }
  }
}

int tokenize(Arena *a, Tokenizer *t, const char *smiles) {
  t->string = smiles;
  size_t string_length = strlen(smiles);

  int last_token;
  while (1) {
    last_token = get_token(a, t, string_length);
    if (last_token == TOKEN_ERR) {
      printf("Error during tokenization\n");
      exit(1);
    }
    if (last_token == TOKEN_END)
      break;
  }

  // for (size_t i = 0; i < t->token_count; i++) {
  //   char buf[64];
  //   printf("%s\n", token_string(&t->tokens[i], buf));
  // }
  //
  return 0;
}

//===============================
//
//   Stack
//
//===============================

#define MAX_STACK_DEPTH 128
typedef struct {
  size_t items[MAX_STACK_DEPTH];
  size_t size;
} NumberStack;

size_t stack_get_count(NumberStack *stack) { return stack->size; }

size_t stack_peek(NumberStack *stack) { return stack->items[stack->size - 1]; }

void stack_push(NumberStack *stack, size_t number) {
  stack->items[stack->size++] = number;
}

size_t stack_pop(NumberStack *stack) { return stack->items[--stack->size]; }
void stack_replace(NumberStack *stack, size_t new_number) {
  stack_pop(stack);
  stack_push(stack, new_number);
}

//===============================
//
//   Parser
//
//===============================

// <smiles>       ::= <chain> | <chain> <div_dot> <smiles>
// <chain>        ::= <atom> | <atom> <bonds> <chain> | <atom> <branch> <chain>
// | <branch> <chain> <atom>         ::= <organic_atom> | "[" <bracket_atom> "]"
// <branch>       ::= "(" <chain> ")" | "(" <bond> <chain> ")"
// <bonds>        ::= "" | "-" | "=" | "#" | "$" | ":" | "/" | "\"
// <div_dot>      ::= "."
//
// <organic_atom> ::= "B" | "C" | "N" | "O" | "P" | "S" | "F" | "Cl" | "Br" |
// "I" | "b" | "c" | "n" | "o" | "s" <bracket_atom> ::= <isotope>? <element>
// <chirality>? <hcount>? <charge>? <isotope>      ::= <number> <element> ::=
// "H" | "Be" | "Ca" | <organic_atom> | ... (other atomic symbols) <chirality>
// ::= "@" | "@@" | "@TH1" | "@TH2" | "@AL1" | "@AL2" | ... (other chiral
// classes) <hcount>       ::= "H" | "H" <digit> <charge>       ::= "-" | "+" |
// "-" <digit> | "+" <digit> | "--" | "++" <bond>         ::= "-" | "=" | "#" |
// "$" | ":" | "/" | "\" <number>       ::= <digit> | <digit> <number> <digit>
// ::= "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"

typedef enum { AST_CHAIN, AST_BRANCH, AST_ATOM, AST_BOND } ast_node_t;

#define MAX_AST_CHILDREN 32
typedef struct ASTNode {
  ast_node_t type;
  struct ASTNode **children;
  size_t num_children;
  union {
    struct {
      char *atom;
      size_t index;
      int label;
    } atom;
    struct {
      int order;
    } bond;
    struct {
      int value;
    } num;
    struct {
      size_t source_atom_index;
    } branch;
  } as;
} ASTNode;

typedef struct {
  Token *tokens;
  size_t token_count;
  size_t pointer;
  size_t atom_count;
  NumberStack last_atoms;
} Parser;

ASTNode *ast_node_create(Arena *a, ast_node_t type) {
  ASTNode *node = arena_alloc(a, sizeof(ASTNode));
  node->type = type;
  node->children = arena_alloc(a, MAX_AST_CHILDREN * sizeof(ASTNode *));
  node->num_children = 0;
  return node;
}

Token peek(Parser *p) { return p->tokens[p->pointer]; }
Token eat(Parser *p, token_t type) {
  Token tok = peek(p);
  if (tok.type != type) {
    Token expected;
    expected.type = type;
    char buf[32];
    printf("[%zu] Expected a: ", p->pointer);
    printf("%s, got %s\n", token_string(&expected, buf),
           token_string(&tok, buf));
    exit(1);
  }
  return p->tokens[p->pointer++];
}

ASTNode *parse_atom(Arena *a, Parser *p) {
  ASTNode *atom_node = ast_node_create(a, AST_ATOM);
  Token atom_token = eat(p, TOKEN_ATOM);
  size_t n = strlen(atom_token.as.atom.atom) + 1;
  atom_node->as.atom.atom = arena_alloc(a, n);
  strcpy(atom_node->as.atom.atom, atom_token.as.atom.atom);
  atom_node->as.atom.index = p->atom_count++;
  atom_node->as.atom.label = -1;
  if (peek(p).type == TOKEN_COLON) {
    eat(p, TOKEN_COLON);
    Token atom_index_token = eat(p, TOKEN_NUMBER);
    atom_node->as.atom.label = atom_index_token.as.num.value;
  }
  return atom_node;
}
ASTNode *parse_bond(Arena *a, Parser *p) {
  ASTNode *bond_node = ast_node_create(a, AST_BOND);
  Token bond = eat(p, TOKEN_BOND);
  bond_node->as.bond.order = bond.as.bond.order;
  return bond_node;
}

ASTNode *parse_chain(Arena *a, Parser *p);
ASTNode *parse_branch(Arena *a, Parser *p) {
  ASTNode *branch_node = ast_node_create(a, AST_BRANCH);
  int branch_bond_order = 1;
  if (peek(p).type == TOKEN_LPAREN) {
    return branch_node;
  }
  ASTNode *chain = parse_chain(a, p);
  branch_node->children[branch_node->num_children++] = chain;
  return branch_node;
}
ASTNode *parse_chain(Arena *a, Parser *p) {
  ASTNode *chain = ast_node_create(a, AST_CHAIN);
  int last_atom = -1;
  while (p->pointer < p->token_count) {
    switch (peek(p).type) {
    case TOKEN_BOND: {
      ASTNode *bond = parse_bond(a, p);
      chain->children[chain->num_children++] = bond;
      break;
    }
    case TOKEN_LBRACKET: {
      eat(p, TOKEN_LBRACKET);
      ASTNode *atom = parse_atom(a, p);
      last_atom = atom->as.atom.index;
      chain->children[chain->num_children++] = atom;
      eat(p, TOKEN_RBRACKET);
      break;
    }
    case TOKEN_LPAREN: {
      eat(p, TOKEN_LPAREN);
      ASTNode *branch = parse_branch(a, p);
      if (last_atom > 0) {
        branch->as.branch.source_atom_index = (int)last_atom;
      } else {
        printf("WARNING: Headless branch detected!\n");
      }
      chain->children[chain->num_children++] = branch;
      eat(p, TOKEN_RPAREN);
      break;
    }
    case TOKEN_RPAREN: {
      return chain;
      break;
    }
    case TOKEN_ATOM: {
      ASTNode *atom = parse_atom(a, p);
      last_atom = atom->as.atom.index;
      chain->children[chain->num_children++] = atom;
      break;
    }
    default: {
      char buf[64];
      Token tok = peek(p);
      printf("Unexpected token: %s", token_string(&tok, buf));
      exit(1);
    }
    }
  }
  return chain;
}

ASTNode *parse_tokens(Arena *a, Token *tokens, size_t token_count) {
  Parser p = {0};
  p.tokens = tokens;
  p.token_count = token_count;
  ASTNode *root = parse_chain(a, &p);
  return root;
}

const char *ast_node_string(ASTNode *node, char *buf, size_t tab_level) {
  for (size_t i = 0; i < tab_level; i++) {
    printf("  ");
  }
  switch (node->type) {
  case AST_ATOM: {
    sprintf(buf, "AST_ATOM: %s:%d [%zu]", node->as.atom.atom,
            node->as.atom.label, node->as.atom.index);
    return buf;
  }
  case AST_BOND: {
    sprintf(buf, "AST_BOND: %d", node->as.bond.order);
    return buf;
  }
  case AST_BRANCH: {
    sprintf(buf, "AST_BRANCH: Head: %zu", node->as.branch.source_atom_index);
    return buf;
  }
  case AST_CHAIN: {
    sprintf(buf, "AST_CHAIN");
    return buf;
  }
  }
}

void print_ast(ASTNode *root, char *buf, size_t indentation_level) {
  const char *node_str = ast_node_string(root, buf, indentation_level);
  printf("%s\n", node_str);
  for (size_t i = 0; i < root->num_children; i++) {
    print_ast(root->children[i], buf, indentation_level + 1);
  }
}

int main() {
  Arena a = arena_create();

  // const char *smiles = "[C:23]C(=[O:2])COSNa";
  const char *smiles = "[C:1][C:2](=[O:3][C:4][C:5](#O))[O:7][C:8]";
  Tokenizer t = {0};
  tokenize(&a, &t, smiles);
  ASTNode *root = parse_tokens(&a, t.tokens, t.token_count);

  char buf[64];
  print_ast(root, buf, 0);

  arena_destroy(&a);

  return 0;
}
