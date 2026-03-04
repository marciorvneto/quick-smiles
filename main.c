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
      size_t atom_a_index;
      size_t atom_b_index;
    } bond;
    struct {
      int value;
    } num;
    struct {
      size_t source_atom_index;
    } branch;
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
  NumberStack last_atoms;
  int ring_openings[MAX_RINGS];
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
      // If there's a last_atom, then we connect it with
      // a simple bond

      if (last_atom > -1) {
        ASTNode *bond = ast_node_create(a, AST_BOND);
        bond->as.bond.order = 1;
        bond->as.bond.atom_a_index = last_atom;
        bond->as.bond.atom_b_index = atom->as.atom.index;
        chain->children[chain->num_children++] = bond;
      }

      last_atom = atom->as.atom.index;
      chain->children[chain->num_children++] = atom;
      break;
    }
    case TOKEN_NUMBER: {
      Token num_token = eat(p, TOKEN_NUMBER);
      int ring_idx = num_token.as.num.value;
      if (p->ring_openings[ring_idx] > -1) {
        // A ring was already open, need to close it
        int source_atom_index = last_atom;
        int target_atom_index = p->ring_openings[ring_idx];

        ASTNode *bond = ast_node_create(a, AST_BOND);
        bond->as.bond.order = 1;
        bond->as.bond.atom_a_index = source_atom_index;
        bond->as.bond.atom_b_index = target_atom_index;

        p->ring_openings[ring_idx] = -1;
        chain->children[chain->num_children++] = bond;
        break;
      }
      p->ring_openings[ring_idx] = last_atom;
      if (last_atom < 0) {
        printf("WARNING: Headless ring detected!\n");
      }
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
    sprintf(buf, "AST_ATOM: %s:%d [%zu]", node->as.atom.atom,
            node->as.atom.label, node->as.atom.index);
    return buf;
  }
  case AST_BOND: {
    sprintf(buf, "AST_BOND: %zu -(%d)- %zu", node->as.bond.atom_a_index,
            node->as.bond.order, node->as.bond.atom_b_index);
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

//==============================
//
//   Molecule structures
//
//==============================

#define DEFAULT_BOND_ARRAY_SIZE 64
#define DEFAULT_ATOMS_ARRAY_SIZE 64

typedef struct {
  const char *name;
  size_t index;
  size_t label;
} Atom;

// This is a sparse incidence matrix entry
// Simple -> 1
// Double -> 2
// Triple -> 3
// ... and so on
typedef struct {
  size_t atom_a;
  size_t atom_b;
  unsigned short order;
} Bond;

typedef struct {
  const char *name;
  Atom **atoms;
  Bond *bonds;
  size_t atoms_count;
  size_t bonds_count;
} Molecule;

Atom *create_atom(Arena *a, const char *name) {
  Atom *atom = arena_alloc(a, sizeof(Atom));
  atom->name = name;
  return atom;
}

Molecule *create_molecule(Arena *a, const char *name) {
  Molecule *m = arena_alloc(a, sizeof(Molecule));
  m->name = name;
  m->bonds = arena_alloc(a, sizeof(Bond) * DEFAULT_BOND_ARRAY_SIZE);
  m->atoms = arena_alloc(a, sizeof(Atom *) * DEFAULT_ATOMS_ARRAY_SIZE);
  m->atoms_count = 0;
  m->bonds_count = 0;
  return m;
}

Molecule *clone_molecule(Arena *a, Molecule *original, const char *clone_name) {
  assert(original->atoms_count <= DEFAULT_ATOMS_ARRAY_SIZE);
  assert(original->bonds_count <= DEFAULT_BOND_ARRAY_SIZE);
  Molecule *new_molecule = create_molecule(a, clone_name);
  new_molecule->atoms_count = original->atoms_count;
  for (size_t a_idx = 0; a_idx < original->atoms_count; a_idx++) {
    // This is fine because all atoms have been preallocated at the
    // start of the program and only get freed after it ends. Atoms
    // are shared.
    new_molecule->atoms[a_idx] = original->atoms[a_idx];
  }
  new_molecule->bonds_count = original->bonds_count;
  for (size_t b_idx = 0; b_idx < original->bonds_count; b_idx++) {
    new_molecule->bonds[b_idx] = original->bonds[b_idx];
  }
  return new_molecule;
}

void push_atom(Molecule *m, Atom *atom) {
  assert(m->atoms_count < DEFAULT_ATOMS_ARRAY_SIZE);
  m->atoms[m->atoms_count++] = atom;
}
void push_bond(Molecule *m, size_t idx_a, size_t idx_b, unsigned short order) {
  assert(m->bonds_count < DEFAULT_BOND_ARRAY_SIZE);
  Bond bond;
  bond.order = order;
  bond.atom_a = idx_a;
  bond.atom_b = idx_b;
  m->bonds[m->bonds_count++] = bond;
}

//==============================
//
//   AST to Molecule
//
//==============================

Atom *make_atom(Arena *a, const char *name, size_t index, int label) {
  Atom *atom = create_atom(a, name);
  atom->index = index;
  atom->label = label;
  return atom;
}

void register_node_atoms(Arena *a, ASTNode *node, Molecule *m) {
  for (size_t i = 0; i < node->num_children; i++) {
    ASTNode *child = node->children[i];
    if (child->type == AST_ATOM) {
      push_atom(m, make_atom(a, child->as.atom.atom, child->as.atom.index,
                             child->as.atom.label));
      continue;
    }
    register_node_atoms(a, child, m);
  }
}

Molecule *molecule_from_ast(Arena *a, ASTNode *tree, const char *name) {
  Molecule *m = create_molecule(a, name);
  register_node_atoms(a, tree, m);
  return m;
}

int main() {
  Arena a = arena_create();

  Atom *H = create_atom(&a, "H");
  Atom *C = create_atom(&a, "C");
  Atom *O = create_atom(&a, "O");
  Atom *N = create_atom(&a, "N");
  Atom *P = create_atom(&a, "P");
  Atom *S = create_atom(&a, "S");

  // const char *smiles = "[C:23]C(=[O:2])COSNa";
  // const char *smiles = "[C:1][C:2](=[O:3][C:4][C:5](#O))[O:7][C:8]";
  const char *smiles = "C1CCC#CC1";
  Tokenizer t = {0};
  tokenize(&a, &t, smiles);
  ASTNode *root = parse_tokens(&a, t.tokens, t.token_count);

  char buf[64];
  print_tokens(t.tokens, t.token_count);
  print_ast(root, buf, 0);

  Molecule *molecule = molecule_from_ast(&a, root, "cyclohexane");

  arena_destroy(&a);

  return 0;
}
