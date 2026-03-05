#ifndef QUICK_SMILES_H
#define QUICK_SMILES_H

#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//===============================
//
//   Arena
//
//===============================

#define QS_MAX_ARENA_SIZE (1024 * 1024) // 1MB
typedef struct {
  void *base;
  size_t offset;
  size_t capacity;
} qs_Arena;

qs_Arena qs_arena_create();
void *qs_arena_alloc(qs_Arena *a, size_t size);
void qs_arena_destroy(qs_Arena *a);

//===============================
//
//   Tokenizer
//
//===============================

typedef enum {
  QS_TOKEN_NUMBER,
  QS_TOKEN_ATOM,
  QS_TOKEN_COLON,
  QS_TOKEN_LPAREN,
  QS_TOKEN_RPAREN,
  QS_TOKEN_LBRACKET,
  QS_TOKEN_RBRACKET,
  QS_TOKEN_BOND,
  QS_TOKEN_PERCENT,

  QS_TOKEN_END,
  QS_TOKEN_ERR,
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
} qs_Token;

#define QS_MAX_TOKENS 1024
typedef struct {
  const char *string;
  size_t pointer;
  qs_Token tokens[QS_MAX_TOKENS];
  size_t token_count;
} qs_Tokenizer;

int qs_tokenize(qs_Arena *a, qs_Tokenizer *t, const char *smiles);
void qs_print_tokens(qs_Token *tokens, size_t token_count);

//===============================
//
//   Parser
//
//===============================

typedef enum {
  QS_AST_CHAIN,
  QS_AST_BRANCH,
  QS_AST_ATOM,
  QS_AST_BOND,
  QS_AST_RING_BOND
} qs_ast_node_t;

#define QS_MAX_QS_AST_CHILDREN 32
typedef struct qs_ASTNode {
  qs_ast_node_t type;
  struct qs_ASTNode **children;
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
} qs_ASTNode;

#define QS_MAX_RINGS 64
typedef struct {
  qs_Token *tokens;
  size_t token_count;
  size_t pointer;
  size_t atom_count;
  int ring_openings[QS_MAX_RINGS];
} qs_Parser;

qs_ASTNode *qs_parse_tokens(qs_Arena *a, qs_Token *tokens, size_t token_count);
void qs_print_ast(qs_ASTNode *root, size_t indentation_level);

//================================================
//         IMPLEMENTATION
//================================================

#ifdef QUICK_SMILES_IMPLEMENTATION
/* #if 1 */

#define LEN(x) sizeof(x) / sizeof(x[0])

static const char *known_two_letter_atoms[30] = {
    // Common in SMILES
    "Cl", "Br", "Na", "Si", "Fe", "Ca", "Al", "Mg", "Cu", "Zn",
    "Mn", "Co", "Ni", "Cr", "Ti", "Li", "Se", "Sn", "Pb", "As",
    "Sb", "Bi", "Ag", "Ba", "Sr", "Pt", "Pd", "Hg", "Mo", "Ge",
};

//===============================
//
//   Arena
//
//===============================

qs_Arena qs_arena_create() {
  qs_Arena a;
  a.base = malloc(QS_MAX_ARENA_SIZE);
  a.offset = 0;
  a.capacity = QS_MAX_ARENA_SIZE;
  return a;
}

void *qs_arena_alloc(qs_Arena *a, size_t size) {
  size_t amount = (size + 7) & ~7;
  if (a->offset + amount < a->capacity) {
    char *addr = (char *)a->base + a->offset;
    a->offset += amount;
    return addr;
  } else {
    printf("qs_Arena ran out of memory\n");
    return NULL;
  }
}

void qs_arena_destroy(qs_Arena *a) { free(a->base); }

//===============================
//
//   Tokenizer
//
//===============================

static const char *token_string(qs_Token *tok, char *buf) {
  switch (tok->type) {
  case QS_TOKEN_NUMBER: {
    sprintf(buf, "QS_TOKEN_NUMBER: %d", tok->as.num.value);
    return buf;
  }
  case QS_TOKEN_ATOM: {
    sprintf(buf, "QS_TOKEN_ATOM: %s", tok->as.atom.atom);
    return buf;
  }
  case QS_TOKEN_COLON:
    return "QS_TOKEN_COLON";
  case QS_TOKEN_LPAREN:
    return "QS_TOKEN_LPAREN";
  case QS_TOKEN_RPAREN:
    return "QS_TOKEN_RPAREN";
  case QS_TOKEN_LBRACKET:
    return "QS_TOKEN_LBRACKET";
  case QS_TOKEN_RBRACKET:
    return "QS_TOKEN_RBRACKET";
  case QS_TOKEN_BOND: {
    sprintf(buf, "QS_TOKEN_BOND: %d", tok->as.bond.order);
    return buf;
  }
  case QS_TOKEN_PERCENT: {
    return "QS_TOKEN_PERCENT";
  }
  case QS_TOKEN_END:
    return "QS_TOKEN_END";
  case QS_TOKEN_ERR:
    return "QS_TOKEN_ERR";
  }
}

static void push_token(qs_Tokenizer *tokenizer, qs_Token *tok) {
  tokenizer->tokens[tokenizer->token_count++] = *tok;
}

static int parse_int(qs_Tokenizer *tokenizer) {
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

static const char *parse_atom_str(qs_Arena *a, qs_Tokenizer *tokenizer) {
  char *atom_string = (char *)qs_arena_alloc(a, 8);
  memset(atom_string, 0, 8);
  char current = tokenizer->string[tokenizer->pointer];
  size_t i = 0;
  if (isalpha(current) && islower(current)) {
    atom_string[i] = current;
    i++;
    current = tokenizer->string[++tokenizer->pointer];
    return atom_string;
  }
  if (isalpha(current) && isupper(current)) {
    char next = tokenizer->string[tokenizer->pointer + 1];
    if (islower(next)) {
      // Check if it's a known combination
      char buf[3];
      buf[0] = current;
      buf[1] = next;
      buf[2] = '\0';
      for (size_t i = 0; i < LEN(known_two_letter_atoms); i++) {
        if ((strcmp(buf, known_two_letter_atoms[i])) == 0) {
          atom_string[0] = current;
          atom_string[1] = next;
          tokenizer->pointer += 2;
          return atom_string;
        }
      }
    }

    // No matches were found, treat it as an isolated atom
    atom_string[i] = current;
    i++;
    current = tokenizer->string[++tokenizer->pointer];
    return atom_string;
  }

  // Fallback -> Treat whatever string comes next as an atom
  while (isalpha(current)) {
    atom_string[i] = current;
    i++;
    current = tokenizer->string[++tokenizer->pointer];
  }
  return atom_string;
}

static int get_token(qs_Arena *a, qs_Tokenizer *tokenizer,
                     size_t string_length) {
  if (tokenizer->pointer == string_length) {
    return QS_TOKEN_END;
  }
  char current = tokenizer->string[tokenizer->pointer];
  switch (current) {
  case '(': {
    qs_Token tok;
    tok.type = QS_TOKEN_LPAREN;
    push_token(tokenizer, &tok);
    tokenizer->pointer++;
    return tok.type;
  }
  case ')': {
    qs_Token tok;
    tok.type = QS_TOKEN_RPAREN;
    push_token(tokenizer, &tok);
    tokenizer->pointer++;
    return tok.type;
  }
  case '[': {
    qs_Token tok;
    tok.type = QS_TOKEN_LBRACKET;
    push_token(tokenizer, &tok);
    tokenizer->pointer++;
    return tok.type;
  }
  case ']': {
    qs_Token tok;
    tok.type = QS_TOKEN_RBRACKET;
    push_token(tokenizer, &tok);
    tokenizer->pointer++;
    return tok.type;
  }
  case ':': {
    qs_Token tok;
    tok.type = QS_TOKEN_COLON;
    push_token(tokenizer, &tok);
    tokenizer->pointer++;
    return tok.type;
  }
  case '-': {
    qs_Token tok;
    tok.type = QS_TOKEN_BOND;
    tok.as.bond.order = 1;
    push_token(tokenizer, &tok);
    tokenizer->pointer++;
    return tok.type;
  }
  case '=': {
    qs_Token tok;
    tok.type = QS_TOKEN_BOND;
    tok.as.bond.order = 2;
    push_token(tokenizer, &tok);
    tokenizer->pointer++;
    return tok.type;
  }
  case '#': {
    qs_Token tok;
    tok.type = QS_TOKEN_BOND;
    tok.as.bond.order = 3;
    push_token(tokenizer, &tok);
    tokenizer->pointer++;
    return tok.type;
  }
  case '$': {
    qs_Token tok;
    tok.type = QS_TOKEN_BOND;
    tok.as.bond.order = 4;
    push_token(tokenizer, &tok);
    tokenizer->pointer++;
    return tok.type;
  }
  case '%': {
    qs_Token tok;
    tok.type = QS_TOKEN_PERCENT;
    push_token(tokenizer, &tok);
    tokenizer->pointer++;
    return tok.type;
  }
  default: {
    if (isdigit(current)) {
      int num = parse_int(tokenizer);
      qs_Token tok;
      tok.type = QS_TOKEN_NUMBER;
      tok.as.num.value = num;
      push_token(tokenizer, &tok);
      return tok.type;
    }
    if (isalpha(current)) {
      const char *atom = parse_atom_str(a, tokenizer);
      qs_Token tok;
      tok.type = QS_TOKEN_ATOM;
      tok.as.atom.atom = atom;
      push_token(tokenizer, &tok);
      return tok.type;
    }
    tokenizer->pointer++;
    return -1;
  }
  }
}

int qs_tokenize(qs_Arena *a, qs_Tokenizer *t, const char *smiles) {
  t->string = smiles;
  size_t string_length = strlen(smiles);

  int last_token;
  while (1) {
    last_token = get_token(a, t, string_length);
    if (last_token == QS_TOKEN_ERR) {
      printf("Error during tokenization\n");
      exit(1);
    }
    if (last_token == QS_TOKEN_END) {
      qs_Token end_token;
      end_token.type = QS_TOKEN_END;
      push_token(t, &end_token);
      break;
    }
  }

  return 0;
}

void qs_print_tokens(qs_Token *tokens, size_t token_count) {
  for (size_t i = 0; i < token_count; i++) {
    char buf[128];
    printf("%s\n", token_string(&tokens[i], buf));
  }
}

//===============================
//
//   qs_Parser
//
//===============================

static qs_ASTNode *ast_node_create(qs_Arena *a, qs_ast_node_t type) {
  qs_ASTNode *node = (qs_ASTNode *)qs_arena_alloc(a, sizeof(qs_ASTNode));
  node->type = type;
  node->children = (qs_ASTNode **)qs_arena_alloc(a, QS_MAX_QS_AST_CHILDREN *
                                                        sizeof(qs_ASTNode *));
  node->num_children = 0;
  return node;
}

static qs_Token peek(qs_Parser *p) { return p->tokens[p->pointer]; }

static qs_Token eat(qs_Parser *p, token_t type) {
  qs_Token tok = peek(p);
  if (tok.type != type) {
    qs_Token expected;
    expected.type = type;
    char buf[32];
    char expected_buf[32];
    printf("[%zu] Expected a: ", p->pointer);
    printf("%s, got %s\n", token_string(&expected, buf),
           token_string(&tok, expected_buf));
    exit(1);
  }
  return p->tokens[p->pointer++];
}

static qs_ASTNode *parse_atom(qs_Arena *a, qs_Parser *p) {
  qs_ASTNode *atom_node = ast_node_create(a, QS_AST_ATOM);
  qs_Token atom_token = eat(p, QS_TOKEN_ATOM);
  size_t n = strlen(atom_token.as.atom.atom) + 1;
  atom_node->as.atom.atom = (char *)qs_arena_alloc(a, n);
  strcpy(atom_node->as.atom.atom, atom_token.as.atom.atom);
  atom_node->as.atom.label = -1;
  if (peek(p).type == QS_TOKEN_COLON) {
    eat(p, QS_TOKEN_COLON);
    qs_Token atom_index_token = eat(p, QS_TOKEN_NUMBER);
    atom_node->as.atom.label = atom_index_token.as.num.value;
  }
  return atom_node;
}
static qs_ASTNode *parse_bond(qs_Arena *a, qs_Parser *p) {
  qs_ASTNode *bond_node = ast_node_create(a, QS_AST_BOND);
  qs_Token bond = eat(p, QS_TOKEN_BOND);
  bond_node->as.bond.order = bond.as.bond.order;
  return bond_node;
}

static void ast_push_child(qs_ASTNode *node, qs_ASTNode *child) {
  node->children[node->num_children++] = child;
}

static int next_digit(int *number) {
  if (*number <= 0)
    return -1;
  int exp = floor(log10(*number));
  int power = pow(10, exp);

  int quotient = floor(*number / power);
  int rem = *number % power;

  *number = rem;
  return quotient;
}

static void process_atom_neighbors(qs_Arena *a, qs_Parser *p, qs_ASTNode *chain,
                                   qs_ASTNode *atom) {
  // Check for ring bonds
  qs_Token next = peek(p);
  if (next.type == QS_TOKEN_PERCENT) {
    eat(p, QS_TOKEN_PERCENT);
    qs_Token number_token = eat(p, QS_TOKEN_NUMBER);
    int number = number_token.as.num.value;
    int digit = -1;
    while ((digit = next_digit(&number)) > -1) {
      qs_ASTNode *ring_bond = ast_node_create(a, QS_AST_RING_BOND);
      ring_bond->as.ring_bond.label = digit;
      ast_push_child(atom, ring_bond);
    }
  } else {
    while (next.type == QS_TOKEN_NUMBER) {
      // Ring bond location
      qs_ASTNode *ring_bond = ast_node_create(a, QS_AST_RING_BOND);
      ring_bond->as.ring_bond.label = next.as.num.value;
      ast_push_child(atom, ring_bond);
      eat(p, QS_TOKEN_NUMBER);
      next = peek(p);
    }
  }

  // Check for implicit bonds
  next = peek(p);
  if (next.type == QS_TOKEN_ATOM || next.type == QS_TOKEN_LBRACKET) {
    // Implicit bond!
    qs_ASTNode *bond = ast_node_create(a, QS_AST_BOND);
    bond->as.bond.order = 1;
    ast_push_child(chain, bond);
  }
}

static qs_ASTNode *parse_chain(qs_Arena *a, qs_Parser *p) {
  qs_ASTNode *chain = ast_node_create(a, QS_AST_CHAIN);
  qs_ASTNode *last_atom = NULL;
  while (p->pointer < p->token_count) {
    switch (peek(p).type) {
    case QS_TOKEN_BOND: {
      qs_ASTNode *bond = parse_bond(a, p);
      ast_push_child(chain, bond);
      break;
    }
    case QS_TOKEN_LBRACKET: {
      eat(p, QS_TOKEN_LBRACKET);

      qs_ASTNode *atom = parse_atom(a, p);
      ast_push_child(chain, atom);
      last_atom = atom;
      eat(p, QS_TOKEN_RBRACKET);
      process_atom_neighbors(a, p, chain, atom);

      break;
    }
    case QS_TOKEN_LPAREN: {
      if (last_atom == NULL) {
        printf("There is no atom for chain %zu to attach.\n", p->pointer);
        break;
      }
      eat(p, QS_TOKEN_LPAREN);
      qs_ASTNode *branch = ast_node_create(a, QS_AST_BRANCH);
      ast_push_child(last_atom, branch);
      qs_Token next = peek(p);
      if (next.type == QS_TOKEN_BOND) {
        qs_ASTNode *bond = parse_bond(a, p);
        ast_push_child(branch, bond);
      } else {
        qs_ASTNode *bond = ast_node_create(a, QS_AST_BOND);
        bond->as.bond.order = 1;
        ast_push_child(branch, bond);
      }
      qs_ASTNode *new_chain = parse_chain(a, p);
      ast_push_child(branch, new_chain);
      break;
    }
    case QS_TOKEN_RPAREN: {
      eat(p, QS_TOKEN_RPAREN);
      return chain;
      break;
    }
    case QS_TOKEN_ATOM: {
      qs_ASTNode *atom = parse_atom(a, p);
      ast_push_child(chain, atom);
      last_atom = atom;
      process_atom_neighbors(a, p, chain, atom);
      break;
    }
    case QS_TOKEN_END: {
      return chain;
    }
    default: {
      char buf[64];
      qs_Token tok = peek(p);
      printf("Unexpected token: %s\n", token_string(&tok, buf));
      exit(1);
    }
    }
  }
  return chain;
}

qs_ASTNode *qs_parse_tokens(qs_Arena *a, qs_Token *tokens, size_t token_count) {
  qs_Parser p = {0};
  for (size_t i = 0; i < QS_MAX_RINGS; i++) {
    p.ring_openings[i] = -1;
  }

  p.tokens = tokens;
  p.token_count = token_count;
  qs_ASTNode *root = parse_chain(a, &p);
  return root;
}

static const char *ast_node_string(qs_ASTNode *node, char *buf,
                                   size_t tab_level) {
  for (size_t i = 0; i < tab_level; i++) {
    printf("  ");
  }
  switch (node->type) {
  case QS_AST_ATOM: {
    if (node->as.atom.label > -1) {
      sprintf(buf, "QS_AST_ATOM: %s:%d", node->as.atom.atom,
              node->as.atom.label);
    } else {
      sprintf(buf, "QS_AST_ATOM: %s", node->as.atom.atom);
    }
    return buf;
  }
  case QS_AST_BOND: {
    sprintf(buf, "QS_AST_BOND: (%d)", node->as.bond.order);
    return buf;
  }
  case QS_AST_BRANCH: {
    sprintf(buf, "QS_AST_BRANCH");
    return buf;
  }
  case QS_AST_CHAIN: {
    sprintf(buf, "QS_AST_CHAIN");
    return buf;
  }
  case QS_AST_RING_BOND: {
    sprintf(buf, "QS_AST_RING_BOND: (%d)", node->as.ring_bond.label);
    return buf;
  }
  }
}

void qs_print_ast(qs_ASTNode *root, size_t indentation_level) {
  char buf[128];
  const char *node_str = ast_node_string(root, buf, indentation_level);
  printf("%s\n", node_str);
  for (size_t i = 0; i < root->num_children; i++) {
    qs_print_ast(root->children[i], indentation_level + 1);
  }
}

#endif // QUICK_SMILES_IMPLEMENTATION
#endif // QUICK_SMILES_H
