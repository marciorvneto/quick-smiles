#ifndef QUICK_SMILES_H
#define QUICK_SMILES_H

#include <assert.h>
#include <ctype.h>
#include <stddef.h>
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

Arena arena_create();
void *arena_alloc(Arena *a, size_t size);
void arena_destroy(Arena *a);

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

const char *token_string(Token *tok, char *buf);

#define MAX_TOKENS 1024
typedef struct {
  const char *string;
  size_t pointer;
  Token tokens[MAX_TOKENS];
  size_t token_count;
} Tokenizer;

void push_token(Tokenizer *tokenizer, Token *tok);
int parse_int(Tokenizer *tokenizer);
const char *parse_atom_str(Arena *a, Tokenizer *tokenizer);
int get_token(Arena *a, Tokenizer *tokenizer, size_t string_length);
int tokenize(Arena *a, Tokenizer *t, const char *smiles);
void print_tokens(Token *tokens, size_t token_count);

//===============================
//
//   Parser
//
//===============================

typedef enum {
  AST_CHAIN,
  AST_BRANCH,
  AST_ATOM,
  AST_BOND,
  AST_RING_BOND
} ast_node_t;

#define MAX_AST_CHILDREN 32
typedef struct ASTNode {
  ast_node_t type;
  struct ASTNode **children;
  size_t num_children;
  union {
    struct {
      char *atom;
      int label;
    } atom;
    struct {
      int order;
    } bond;
    struct {
      int label;
    } ring_bond;
    struct {
      size_t source_atom_index;
      size_t target_atom_index;
    } bridge;
  } as;
} ASTNode;

#define MAX_RINGS 64
typedef struct {
  Token *tokens;
  size_t token_count;
  size_t pointer;
  size_t atom_count;
  int ring_openings[MAX_RINGS];
} Parser;

ASTNode *ast_node_create(Arena *a, ast_node_t type);
Token peek(Parser *p);
Token eat(Parser *p, token_t type);
ASTNode *parse_atom(Arena *a, Parser *p);
ASTNode *parse_bond(Arena *a, Parser *p);
void ast_push_child(ASTNode *node, ASTNode *child);
void process_atom_neighbors(Arena *a, Parser *p, ASTNode *chain, ASTNode *atom);
ASTNode *parse_chain(Arena *a, Parser *p);
ASTNode *parse_tokens(Arena *a, Token *tokens, size_t token_count);
const char *ast_node_string(ASTNode *node, char *buf, size_t tab_level);
void print_ast(ASTNode *root, char *buf, size_t indentation_level);

#ifdef QUICK_SMILES_IMPLEMENTATION

//===============================
//
//   Arena
//
//===============================

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
    char *addr = (char *)a->base + a->offset;
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

void push_token(Tokenizer *tokenizer, Token *tok) {
  tokenizer->tokens[tokenizer->token_count++] = *tok;
}

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
  char *atom_string = (char *)arena_alloc(a, 8);
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
    push_token(tokenizer, &tok);
    tokenizer->pointer++;
    return tok.type;
  }
  case ')': {
    Token tok;
    tok.type = TOKEN_RPAREN;
    push_token(tokenizer, &tok);
    tokenizer->pointer++;
    return tok.type;
  }
  case '[': {
    Token tok;
    tok.type = TOKEN_LBRACKET;
    push_token(tokenizer, &tok);
    tokenizer->pointer++;
    return tok.type;
  }
  case ']': {
    Token tok;
    tok.type = TOKEN_RBRACKET;
    push_token(tokenizer, &tok);
    tokenizer->pointer++;
    return tok.type;
  }
  case ':': {
    Token tok;
    tok.type = TOKEN_COLON;
    push_token(tokenizer, &tok);
    tokenizer->pointer++;
    return tok.type;
  }
  case '-': {
    Token tok;
    tok.type = TOKEN_BOND;
    tok.as.bond.order = 1;
    push_token(tokenizer, &tok);
    tokenizer->pointer++;
    return tok.type;
  }
  case '=': {
    Token tok;
    tok.type = TOKEN_BOND;
    tok.as.bond.order = 2;
    push_token(tokenizer, &tok);
    tokenizer->pointer++;
    return tok.type;
  }
  case '#': {
    Token tok;
    tok.type = TOKEN_BOND;
    tok.as.bond.order = 3;
    push_token(tokenizer, &tok);
    tokenizer->pointer++;
    return tok.type;
  }
  case '$': {
    Token tok;
    tok.type = TOKEN_BOND;
    tok.as.bond.order = 4;
    push_token(tokenizer, &tok);
    tokenizer->pointer++;
    return tok.type;
  }
  default: {
    if (isdigit(current)) {
      int num = parse_int(tokenizer);
      Token tok;
      tok.type = TOKEN_NUMBER;
      tok.as.num.value = num;
      push_token(tokenizer, &tok);
      return tok.type;
    }
    if (isalpha(current)) {
      const char *atom = parse_atom_str(a, tokenizer);
      Token tok;
      tok.type = TOKEN_ATOM;
      tok.as.atom.atom = atom;
      push_token(tokenizer, &tok);
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

  return 0;
}

void print_tokens(Token *tokens, size_t token_count) {
  for (size_t i = 0; i < token_count; i++) {
    char buf[128];
    printf("%s\n", token_string(&tokens[i], buf));
  }
}

//===============================
//
//   Parser
//
//===============================

ASTNode *ast_node_create(Arena *a, ast_node_t type) {
  ASTNode *node = (ASTNode *)arena_alloc(a, sizeof(ASTNode));
  node->type = type;
  node->children =
      (ASTNode **)arena_alloc(a, MAX_AST_CHILDREN * sizeof(ASTNode *));
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
  atom_node->as.atom.atom = (char *)arena_alloc(a, n);
  strcpy(atom_node->as.atom.atom, atom_token.as.atom.atom);
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

void ast_push_child(ASTNode *node, ASTNode *child) {
  node->children[node->num_children++] = child;
}

void process_atom_neighbors(Arena *a, Parser *p, ASTNode *chain,
                            ASTNode *atom) {
  // Check for ring bonds
  Token next = peek(p);
  if (next.type == TOKEN_NUMBER) {
    // Ring bond location
    ASTNode *ring_bond = ast_node_create(a, AST_RING_BOND);
    ring_bond->as.ring_bond.label = next.as.num.value;
    ast_push_child(atom, ring_bond);
    eat(p, TOKEN_NUMBER);
  }

  // Check for implicit bonds
  next = peek(p);
  if (next.type == TOKEN_ATOM || next.type == TOKEN_LBRACKET) {
    // Implicit bond!
    ASTNode *bond = ast_node_create(a, AST_BOND);
    bond->as.bond.order = 1;
    ast_push_child(chain, bond);
  }
}

ASTNode *parse_chain(Arena *a, Parser *p) {
  ASTNode *chain = ast_node_create(a, AST_CHAIN);
  ASTNode *last_atom = NULL;
  while (p->pointer < p->token_count) {
    switch (peek(p).type) {
    case TOKEN_BOND: {
      ASTNode *bond = parse_bond(a, p);
      ast_push_child(chain, bond);
      break;
    }
    case TOKEN_LBRACKET: {
      eat(p, TOKEN_LBRACKET);

      ASTNode *atom = parse_atom(a, p);
      ast_push_child(chain, atom);
      last_atom = atom;
      eat(p, TOKEN_RBRACKET);
      process_atom_neighbors(a, p, chain, atom);

      break;
    }
    case TOKEN_LPAREN: {
      if (last_atom == NULL) {
        printf("There is no atom for chain %zu to attach.\n", p->pointer);
        break;
      }
      eat(p, TOKEN_LPAREN);
      ASTNode *branch = ast_node_create(a, AST_BRANCH);
      ast_push_child(last_atom, branch);
      Token next = peek(p);
      if (next.type == TOKEN_BOND) {
        ASTNode *bond = parse_bond(a, p);
        ast_push_child(branch, bond);
      } else {
        ASTNode *bond = ast_node_create(a, AST_BOND);
        bond->as.bond.order = 1;
        ast_push_child(branch, bond);
      }
      ASTNode *new_chain = parse_chain(a, p);
      ast_push_child(branch, new_chain);
      break;
    }
    case TOKEN_RPAREN: {
      eat(p, TOKEN_RPAREN);
      return chain;
      break;
    }
    case TOKEN_ATOM: {
      ASTNode *atom = parse_atom(a, p);
      ast_push_child(chain, atom);
      last_atom = atom;
      process_atom_neighbors(a, p, chain, atom);
      break;
    }
    default: {
      char buf[64];
      Token tok = peek(p);
      printf("Unexpected token: %s\n", token_string(&tok, buf));
      exit(1);
    }
    }
  }
  return chain;
}

ASTNode *parse_tokens(Arena *a, Token *tokens, size_t token_count) {
  Parser p = {0};
  for (size_t i = 0; i < MAX_RINGS; i++) {
    p.ring_openings[i] = -1;
  }

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
    if (node->as.atom.label > -1) {
      sprintf(buf, "AST_ATOM: %s:%d", node->as.atom.atom, node->as.atom.label);
    } else {
      sprintf(buf, "AST_ATOM: %s", node->as.atom.atom);
    }
    return buf;
  }
  case AST_BOND: {
    sprintf(buf, "AST_BOND: (%d)", node->as.bond.order);
    return buf;
  }
  case AST_BRANCH: {
    sprintf(buf, "AST_BRANCH");
    return buf;
  }
  case AST_CHAIN: {
    sprintf(buf, "AST_CHAIN");
    return buf;
  }
  case AST_RING_BOND: {
    sprintf(buf, "AST_RING_BOND");
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

#endif // QUICK_SMILES_IMPLEMENTATION
#endif // QUICK_SMILES_H
