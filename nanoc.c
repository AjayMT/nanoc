
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

FILE *input = NULL;

typedef enum {
  INT_LITERAL, SEMICOLON, EOF_, LPAREN, RPAREN, LCURLY, RCURLY,
  COMMA, LT, GT, LTE, GTE, EQ, EQUAL, NEQUAL, NOT, BIT_AND, BIT_OR,
  BIT_NOT, BIT_XOR, AND, OR, PLUS, MINUS, ASTERISK, SLASH, PERCENT,
  PLUS_EQ, MINUS_EQ, ASTERISK_EQ, SLASH_EQ, PERCENT_EQ, PLUS_PLUS, MINUS_MINUS,
  AND_EQ, OR_EQ, XOR_EQ, NOT_EQ,
  CHAR_LITERAL, STR_LITERAL, IDENT,
  INT, VOID, CHAR, IF, ELSE, WHILE, CONTINUE, BREAK, RETURN
} token_type_t;

typedef struct token_s {
  token_type_t type;
  char *str;
  uint32_t pos;
} token_t;

char escape_char(char c)
{
  switch (c) {
  case 'n': return '\n';
  case 't': return '\t';
  }
  return c;
}

uint8_t buffered_token = 0;
token_t tok_buf;

token_t next_token()
{
  if (buffered_token) {
    buffered_token = 0;
    return tok_buf;
  }

  char buf[512]; // max token size is 512
  memset(buf, 0, 512);
  uint32_t buf_idx = 0;
  char c = getc(input);

  while (isspace(c)) c = getc(input); // skip spaces

#define RET_EMPTY_TOKEN(t)                                              \
  return (token_t) { .type = (t), .str = NULL, .pos = ftell(input) }

  switch (c) {
  case EOF: RET_EMPTY_TOKEN(EOF_);
  case ';': RET_EMPTY_TOKEN(SEMICOLON);
  case '(': RET_EMPTY_TOKEN(LPAREN);
  case ')': RET_EMPTY_TOKEN(RPAREN);
  case '{': RET_EMPTY_TOKEN(LCURLY);
  case '}': RET_EMPTY_TOKEN(RCURLY);
  case ',': RET_EMPTY_TOKEN(COMMA);
  }

  if (c == '<') {
    c = getc(input);
    if (c == '=') RET_EMPTY_TOKEN(LTE);
    ungetc(c, input); RET_EMPTY_TOKEN(LT);
  }

  if (c == '>') {
    c = getc(input);
    if (c == '=') RET_EMPTY_TOKEN(GTE);
    ungetc(c, input); RET_EMPTY_TOKEN(GT);
  }

  if (c == '=') {
    c = getc(input);
    if (c == '=') RET_EMPTY_TOKEN(EQUAL);
    ungetc(c, input); RET_EMPTY_TOKEN(EQ);
  }

  if (c == '!') {
    c = getc(input);
    if (c == '=') RET_EMPTY_TOKEN(NEQUAL);
    ungetc(c, input); RET_EMPTY_TOKEN(NOT);
  }

  if (c == '&') {
    c = getc(input);
    if (c == '&') RET_EMPTY_TOKEN(AND);
    if (c == '=') RET_EMPTY_TOKEN(AND_EQ);
    ungetc(c, input); RET_EMPTY_TOKEN(BIT_AND);
  }

  if (c == '|') {
    c = getc(input);
    if (c == '|') RET_EMPTY_TOKEN(OR);
    if (c == '=') RET_EMPTY_TOKEN(OR_EQ);
    ungetc(c, input); RET_EMPTY_TOKEN(BIT_OR);
  }

  if (c == '^') {
    c = getc(input);
    if (c == '=') RET_EMPTY_TOKEN(XOR_EQ);
    ungetc(c, input); RET_EMPTY_TOKEN(BIT_XOR);
  }

  if (c == '~') {
    c = getc(input);
    if (c == '=') RET_EMPTY_TOKEN(NOT_EQ);
    ungetc(c, input); RET_EMPTY_TOKEN(BIT_NOT);
  }

  if (c == '+') {
    c = getc(input);
    if (c == '=') RET_EMPTY_TOKEN(PLUS_EQ);
    if (c == '+') RET_EMPTY_TOKEN(PLUS_PLUS);
    ungetc(c, input); RET_EMPTY_TOKEN(PLUS);
  }

  if (c == '-') {
    c = getc(input);
    if (c == '=') RET_EMPTY_TOKEN(MINUS_EQ);
    if (c == '-') RET_EMPTY_TOKEN(MINUS_MINUS);
    ungetc(c, input); RET_EMPTY_TOKEN(MINUS);
  }

  if (c == '*') {
    c = getc(input);
    if (c == '=') RET_EMPTY_TOKEN(ASTERISK_EQ);
    ungetc(c, input); RET_EMPTY_TOKEN(ASTERISK);
  }

  if (c == '/') {
    c = getc(input);
    if (c == '=') RET_EMPTY_TOKEN(SLASH_EQ);
    ungetc(c, input); RET_EMPTY_TOKEN(SLASH);
  }

  if (c == '%') {
    c = getc(input);
    if (c == '=') RET_EMPTY_TOKEN(PERCENT_EQ);
    ungetc(c, input); RET_EMPTY_TOKEN(PERCENT);
  }

  if (c == '\'') {
    c = getc(input);
    uint8_t escaped = 0;
    if (c == '\\') {
      escaped = 1;
      c = getc(input);
    }
    if (c == '\'' && !escaped) goto fail;
    if (escaped) c = escape_char(c);
    buf[buf_idx] = c; ++buf_idx;
    c = getc(input);
    if (c != '\'') goto fail;
    char *s = strndup(buf, buf_idx);
    return (token_t) { .type = CHAR_LITERAL, .str = s, .pos = ftell(input) };
  }

  if (c == '"') {
    c = getc(input);
    uint8_t escaped = 0;
    while (c != '"' || escaped) {
      if (c == '\n' || c == '\r') goto fail;
      if (escaped) c = escape_char(c);
      // TODO: handle "\\"
      escaped = 0;
      if (c == '\\') escaped = 1;
      else { buf[buf_idx] = c; ++buf_idx; }
      c = getc(input);
    }
    char *s = strndup(buf, buf_idx);
    return (token_t) { .type = STR_LITERAL, .str = s, .pos = ftell(input) };
  }

  if (isdigit(c)) {
    while (isdigit(c)) {
      buf[buf_idx] = c; ++buf_idx;
      c = getc(input);
    }
    ungetc(c, input);
    char *s = strndup(buf, buf_idx);
    return (token_t) { .type = INT_LITERAL, .str = s, .pos = ftell(input) };
  }

  if (isalpha(c)) {
    while (isalnum(c) || c == '_') {
      buf[buf_idx] = c; ++buf_idx;
      c = getc(input);
    }
    ungetc(c, input);

    if (buf_idx == 3 && strncmp(buf, "int", 3) == 0) RET_EMPTY_TOKEN(INT);
    if (buf_idx == 4 && strncmp(buf, "char", 4) == 0) RET_EMPTY_TOKEN(CHAR);
    if (buf_idx == 4 && strncmp(buf, "void", 4) == 0) RET_EMPTY_TOKEN(VOID);
    if (buf_idx == 2 && strncmp(buf, "if", 2) == 0) RET_EMPTY_TOKEN(IF);
    if (buf_idx == 4 && strncmp(buf, "else", 4) == 0) RET_EMPTY_TOKEN(ELSE);
    if (buf_idx == 5 && strncmp(buf, "while", 5) == 0) RET_EMPTY_TOKEN(WHILE);
    if (buf_idx == 8 && strncmp(buf, "continue", 8) == 0) RET_EMPTY_TOKEN(CONTINUE);
    if (buf_idx == 5 && strncmp(buf, "break", 5) == 0) RET_EMPTY_TOKEN(BREAK);
    if (buf_idx == 6 && strncmp(buf, "return", 6) == 0) RET_EMPTY_TOKEN(RETURN);

    char *s = strndup(buf, buf_idx);
    return (token_t) { .type = IDENT, .str = s, .pos = ftell(input) };
  }

fail:
  printf("Malformed token: %s\nAt position %lu\n", buf, ftell(input));
  exit(1);

  return (token_t) { }; // to suppress compiler warning
}

void buffer_token(token_t tok)
{
  buffered_token = 1;
  tok_buf = tok;
}

typedef enum {
  nFUNCTION, nARGUMENT, nSTMT, nEXPR, nTYPE
} ast_node_type_t;

typedef enum {
  // nSTMT variants
  vDECL, vEXPR, vEMPTY, vBLOCK, vIF, vWHILE, vCONTINUE, vBREAK, vRETURN,

  // nEXPR variants
  vIDENT, vINT_LITERAL, vCHAR_LITERAL, vSTRING_LITERAL,
  vDEREF, vADDRESSOF, vINCREMENT, vDECREMENT, vNOT,
  vADD, vSUBTRACT, vMULTIPLY, vDIVIDE, vMODULO,
  vLT, vGT, vEQUAL, vAND, vOR,
  vBIT_AND, vBIT_OR, vBIT_XOR, vBIT_NOT,
  vASSIGN, vCALL,

  // nTYPE variants
  vINT, vCHAR, vVOID, vPTR
} ast_node_variant_t;

typedef struct ast_node_s {
  ast_node_type_t type;
  ast_node_variant_t variant;
  int32_t i;
  char *s;
  struct ast_node_s *children;
  struct ast_node_s *next;
} ast_node_t;

ast_node_t *parse_type()
{
  token_t tok = next_token();
  if (tok.type != INT && tok.type != CHAR && tok.type != VOID) {
    printf("Expected type at position %u\n", tok.pos);
    exit(1);
  }

  ast_node_t *root = malloc(sizeof(ast_node_t));
  memset(root, 0, sizeof(ast_node_t));
  root->type = nTYPE;
  if (tok.type == INT) root->variant = vINT;
  else if (tok.type == CHAR) root->variant = vCHAR;
  else root->variant = vVOID;

  tok = next_token();
  while (tok.type == ASTERISK) {
    ast_node_t *ptr_type = malloc(sizeof(ast_node_t));
    memset(ptr_type, 0, sizeof(ast_node_t));
    ptr_type->type = nTYPE;
    ptr_type->variant = vPTR;
    ptr_type->children = root;
    root = ptr_type;
    tok = next_token();
  }
  buffer_token(tok);

  return root;
}

void check_lval(ast_node_t *root, uint32_t pos)
{
  if (root == NULL) {
    printf("Invalid lvalue at position %u\n", pos);
    exit(1);
  }
  if (root->type == nEXPR && root->variant == vIDENT)
    return;
  if (root->type == nEXPR && root->variant == vDEREF) {
    check_lval(root->children, pos);
    return;
  }

  printf("Invalid lvalue at position %u\n", pos);
  exit(1);
}

ast_node_t *parse_expr()
{
  ast_node_t *root;
  token_t tok = next_token();

#define PARSE_BINOP(tok_type, variant_type, transform)  \
  if (tok.type == (tok_type)) {                         \
    ast_node_t *left = root;                            \
    ast_node_t *right = parse_expr();                   \
    root = malloc(sizeof(ast_node_t));                  \
    memset(root, 0, sizeof(ast_node_t));                \
    root->type = nEXPR;                                 \
    root->variant = (variant_type);                     \
    left->next = right;                                 \
    root->children = left;                              \
    transform;                                          \
    return root;                                        \
  }                                                     \

  if (tok.type == LPAREN) {
    root = parse_expr();
    tok = next_token();
    if (tok.type != RPAREN) {
      printf("Expected ')' at position %u\n", tok.pos);
      exit(1);
    }

  parse_call:
    tok = next_token();
    if (tok.type == LPAREN) {
      ast_node_t *callee = root;
      root = malloc(sizeof(ast_node_t));
      memset(root, 0, sizeof(ast_node_t));
      root->type = nEXPR;
      root->variant = vCALL;
      root->children = callee;

      ast_node_t **current_arg = &(callee->next);

      tok = next_token();
      while (tok.type != RPAREN) {
        buffer_token(tok);
        *current_arg = parse_expr();
        tok = next_token();
        if (tok.type == COMMA) tok = next_token();
        current_arg = &((*current_arg)->next);
      }
    } else buffer_token(tok);

  parse_binops:
    tok = next_token();

    PARSE_BINOP(PLUS, vADD, {});
    PARSE_BINOP(MINUS, vSUBTRACT, {});
    PARSE_BINOP(ASTERISK, vMULTIPLY, {});
    PARSE_BINOP(SLASH, vDIVIDE, {});
    PARSE_BINOP(PERCENT, vMODULO, {});
    PARSE_BINOP(LT, vLT, {});
    PARSE_BINOP(GT, vGT, {});
    PARSE_BINOP(EQUAL, vEQUAL, {});
    PARSE_BINOP(LTE, vGT, {
        ast_node_t *not_node = malloc(sizeof(ast_node_t));
        memset(not_node, 0, sizeof(ast_node_t));
        not_node->type = nEXPR;
        not_node->variant = vNOT;
        not_node->children = root;
        root = not_node;
      });
    PARSE_BINOP(GTE, vLT, {
        ast_node_t *not_node = malloc(sizeof(ast_node_t));
        memset(not_node, 0, sizeof(ast_node_t));
        not_node->type = nEXPR;
        not_node->variant = vNOT;
        not_node->children = root;
        root = not_node;
      });
    PARSE_BINOP(NEQUAL, vEQUAL, {
        ast_node_t *not_node = malloc(sizeof(ast_node_t));
        memset(not_node, 0, sizeof(ast_node_t));
        not_node->type = nEXPR;
        not_node->variant = vNOT;
        not_node->children = root;
        root = not_node;
      });
    PARSE_BINOP(AND, vAND, {});
    PARSE_BINOP(OR, vOR, {});
    PARSE_BINOP(BIT_AND, vBIT_AND, {});
    PARSE_BINOP(BIT_OR, vBIT_OR, {});
    PARSE_BINOP(BIT_XOR, vBIT_XOR, {});
    PARSE_BINOP(EQ, vASSIGN, { check_lval(left, tok.pos); });
    PARSE_BINOP(PLUS_EQ, vADD, {
        check_lval(left, tok.pos);
        ast_node_t *new_left = malloc(sizeof(ast_node_t));
        memcpy(new_left, left, sizeof(ast_node_t));
        new_left->next = root;
        ast_node_t *new_root = malloc(sizeof(ast_node_t));
        memset(new_root, 0, sizeof(ast_node_t));
        new_root->type = nEXPR;
        new_root->variant = vASSIGN;
        new_root->children = new_left;
        root = new_root;
      });
    PARSE_BINOP(MINUS_EQ, vSUBTRACT, {
        check_lval(left, tok.pos);
        ast_node_t *new_left = malloc(sizeof(ast_node_t));
        memcpy(new_left, left, sizeof(ast_node_t));
        new_left->next = root;
        ast_node_t *new_root = malloc(sizeof(ast_node_t));
        memset(new_root, 0, sizeof(ast_node_t));
        new_root->type = nEXPR;
        new_root->variant = vASSIGN;
        new_root->children = new_left;
        root = new_root;
      });
    PARSE_BINOP(ASTERISK_EQ, vMULTIPLY, {
        check_lval(left, tok.pos);
        ast_node_t *new_left = malloc(sizeof(ast_node_t));
        memcpy(new_left, left, sizeof(ast_node_t));
        new_left->next = root;
        ast_node_t *new_root = malloc(sizeof(ast_node_t));
        memset(new_root, 0, sizeof(ast_node_t));
        new_root->type = nEXPR;
        new_root->variant = vASSIGN;
        new_root->children = new_left;
        root = new_root;
      });
    PARSE_BINOP(SLASH_EQ, vDIVIDE, {
        check_lval(left, tok.pos);
        ast_node_t *new_left = malloc(sizeof(ast_node_t));
        memcpy(new_left, left, sizeof(ast_node_t));
        new_left->next = root;
        ast_node_t *new_root = malloc(sizeof(ast_node_t));
        memset(new_root, 0, sizeof(ast_node_t));
        new_root->type = nEXPR;
        new_root->variant = vASSIGN;
        new_root->children = new_left;
        root = new_root;
      });
    PARSE_BINOP(PERCENT_EQ, vMODULO, {
        check_lval(left, tok.pos);
        ast_node_t *new_left = malloc(sizeof(ast_node_t));
        memcpy(new_left, left, sizeof(ast_node_t));
        new_left->next = root;
        ast_node_t *new_root = malloc(sizeof(ast_node_t));
        memset(new_root, 0, sizeof(ast_node_t));
        new_root->type = nEXPR;
        new_root->variant = vASSIGN;
        new_root->children = new_left;
        root = new_root;
      });
    PARSE_BINOP(AND_EQ, vBIT_AND, {
        check_lval(left, tok.pos);
        ast_node_t *new_left = malloc(sizeof(ast_node_t));
        memcpy(new_left, left, sizeof(ast_node_t));
        new_left->next = root;
        ast_node_t *new_root = malloc(sizeof(ast_node_t));
        memset(new_root, 0, sizeof(ast_node_t));
        new_root->type = nEXPR;
        new_root->variant = vASSIGN;
        new_root->children = new_left;
        root = new_root;
      });
    PARSE_BINOP(OR_EQ, vBIT_OR, {
        check_lval(left, tok.pos);
        ast_node_t *new_left = malloc(sizeof(ast_node_t));
        memcpy(new_left, left, sizeof(ast_node_t));
        new_left->next = root;
        ast_node_t *new_root = malloc(sizeof(ast_node_t));
        memset(new_root, 0, sizeof(ast_node_t));
        new_root->type = nEXPR;
        new_root->variant = vASSIGN;
        new_root->children = new_left;
        root = new_root;
      });
    PARSE_BINOP(XOR_EQ, vBIT_XOR, {
        check_lval(left, tok.pos);
        ast_node_t *new_left = malloc(sizeof(ast_node_t));
        memcpy(new_left, left, sizeof(ast_node_t));
        new_left->next = root;
        ast_node_t *new_root = malloc(sizeof(ast_node_t));
        memset(new_root, 0, sizeof(ast_node_t));
        new_root->type = nEXPR;
        new_root->variant = vASSIGN;
        new_root->children = new_left;
        root = new_root;
      });
    PARSE_BINOP(NOT_EQ, vBIT_NOT, {
        check_lval(left, tok.pos);
        ast_node_t *new_left = malloc(sizeof(ast_node_t));
        memcpy(new_left, left, sizeof(ast_node_t));
        new_left->next = root;
        ast_node_t *new_root = malloc(sizeof(ast_node_t));
        memset(new_root, 0, sizeof(ast_node_t));
        new_root->type = nEXPR;
        new_root->variant = vASSIGN;
        new_root->children = new_left;
        root = new_root;
      });

    buffer_token(tok);

    return root;
  }

  root = malloc(sizeof(ast_node_t));
  memset(root, 0, sizeof(ast_node_t));
  root->type = nEXPR;

  if (tok.type == INT_LITERAL) {
    root->variant = vINT_LITERAL;
    root->i = atoi(tok.str);
    goto parse_binops;
  }

  if (tok.type == CHAR_LITERAL) {
    root->variant = vCHAR_LITERAL;
    root->i = tok.str[0];
    goto parse_binops;
  }

  if (tok.type == STR_LITERAL) {
    root->variant = vSTRING_LITERAL;
    root->s = tok.str;
    goto parse_binops;
  }

  if (tok.type == IDENT) {
    root->variant = vIDENT;
    root->s = tok.str;
    goto parse_call;
  }

  if (tok.type == BIT_AND) {
    ast_node_t *child = parse_expr();
    root->variant = vADDRESSOF;
    root->children = child;
    goto parse_binops;
  }

  if (tok.type == ASTERISK) {
    ast_node_t *child = parse_expr();
    root->variant = vDEREF;
    root->children = child;
    goto parse_binops;
  }

  if (tok.type == BIT_NOT) {
    ast_node_t *child = parse_expr();
    root->variant = vBIT_NOT;
    root->children = child;
    goto parse_binops;
  }

  if (tok.type == NOT) {
    ast_node_t *child = parse_expr();
    root->variant = vNOT;
    root->children = child;
    goto parse_binops;
  }

  if (tok.type == PLUS_PLUS) {
    ast_node_t *child = parse_expr();
    root->variant = vINCREMENT;
    root->children = child;
    goto parse_binops;
  }

  if (tok.type == MINUS_MINUS) {
    ast_node_t *child = parse_expr();
    root->variant = vDECREMENT;
    root->children = child;
    goto parse_binops;
  }

  printf("Malformed expression at position %u\n", tok.pos);
  exit(1);
  return root;
}

ast_node_t *parse_stmt()
{
  ast_node_t *root = malloc(sizeof(ast_node_t));
  memset(root, 0, sizeof(ast_node_t));
  root->type = nSTMT;
  token_t tok = next_token();

  if (tok.type == SEMICOLON) {
    root->variant = vEMPTY;
    return root;
  }

  if (tok.type == CONTINUE) {
    root->variant = vCONTINUE;
    tok = next_token();
    if (tok.type != SEMICOLON) goto fail;
    return root;
  }
  if (tok.type == BREAK) {
    root->variant = vBREAK;
    tok = next_token();
    if (tok.type != SEMICOLON) goto fail;
    return root;
  }

  if (tok.type == IF) {
    root->variant = vIF;
    tok = next_token();
    if (tok.type != LPAREN) goto fail;
    ast_node_t *cond_expr = parse_expr();
    tok = next_token();
    if (tok.type != RPAREN) goto fail;
    ast_node_t *if_stmt = parse_stmt();
    tok = next_token();
    ast_node_t *else_stmt;
    if (tok.type == ELSE) else_stmt = parse_stmt();
    else {
      buffer_token(tok);
      else_stmt = malloc(sizeof(ast_node_t));
      memset(else_stmt, 0, sizeof(ast_node_t));
      else_stmt->type = nSTMT;
      else_stmt->variant = vEMPTY;
    }

    cond_expr->next = if_stmt;
    if_stmt->next = else_stmt;
    root->children = cond_expr;
    return root;
  }

  if (tok.type == WHILE) {
    root->variant = vWHILE;
    tok = next_token();
    if (tok.type != LPAREN) goto fail;
    ast_node_t *cond_expr = parse_expr();
    tok = next_token();
    if (tok.type != RPAREN) goto fail;
    ast_node_t *while_stmt = parse_stmt();

    cond_expr->next = while_stmt;
    root->children = cond_expr;
    return root;
  }

  if (tok.type == RETURN) {
    root->variant = vRETURN;
    tok = next_token();
    if (tok.type == SEMICOLON) return root;
    buffer_token(tok);
    ast_node_t *return_expr = parse_expr();
    root->children = return_expr;
    tok = next_token();
    if (tok.type != SEMICOLON) goto fail;
    return root;
  }

  if (tok.type == LCURLY) {
    root->variant = vBLOCK;
    ast_node_t **child = &(root->children);
    tok = next_token();
    while (tok.type != RCURLY) {
      buffer_token(tok);
      *child = parse_stmt();
      child = &((*child)->next);
      tok = next_token();
    }

    return root;
  }

  if (tok.type == INT || tok.type == CHAR || tok.type == VOID) {
    buffer_token(tok);
    root->variant = vDECL;
    ast_node_t *type_node = parse_type();
    tok = next_token();
    if (tok.type != IDENT) goto fail;
    root->s = tok.str;
    root->children = type_node;
    tok = next_token();
    if (tok.type != SEMICOLON) goto fail;
    return root;
  }

  buffer_token(tok);
  root->variant = vEXPR;
  ast_node_t *expr_node = parse_expr();
  root->children = expr_node;
  tok = next_token();
  if (tok.type != SEMICOLON) {
  fail:
    printf("Malformed statement at position %u\n", tok.pos);
    exit(1);
  }

  return root;
}

ast_node_t *parse()
{
  ast_node_t *root;
  ast_node_t **current = &root;
  token_t tok = next_token();

  while (tok.type != EOF_) {
    buffer_token(tok);
    ast_node_t *type_node = parse_type();
    tok = next_token();
    if (tok.type != IDENT) {
    fail:
      printf("Malformed declaration at position %u\n", tok.pos);
      exit(1);
    }
    char *name = tok.str;
    tok = next_token();
    if (tok.type == SEMICOLON) {
      *current = malloc(sizeof(ast_node_t));
      memset(*current, 0, sizeof(ast_node_t));
      (*current)->type = nSTMT;
      (*current)->variant = vDECL;
      (*current)->s = name;
      (*current)->children = type_node;
      current = &((*current)->next);
      tok = next_token();
      continue;
    }

    if (tok.type != LPAREN) goto fail;

    *current = malloc(sizeof(ast_node_t));
    memset(*current, 0, sizeof(ast_node_t));
    (*current)->type = nFUNCTION;
    (*current)->s = name;
    (*current)->children = type_node;

    ast_node_t **current_arg = &(type_node->next);

    tok = next_token();
    while (tok.type != RPAREN) {
      buffer_token(tok);
      ast_node_t *arg_type_node = parse_type();
      tok = next_token();
      if (tok.type != IDENT) goto fail;
      *current_arg = malloc(sizeof(ast_node_t));
      memset(*current_arg, 0, sizeof(ast_node_t));
      (*current_arg)->type = nARGUMENT;
      (*current_arg)->s = tok.str;
      (*current_arg)->children = arg_type_node;
      current_arg = &((*current_arg)->next);

      tok = next_token();
      if (tok.type == COMMA) tok = next_token();
    }

    *current_arg = parse_stmt();
    if ((*current_arg)->variant != vBLOCK) goto fail;

    current = &((*current)->next);
    tok = next_token();
  }

  return root;
}

int main(int argc, char *argv[])
{
  if (argc < 2) {
    printf("Usage: nanoc <filename>\n");
    return 1;
  }

  char *filename = argv[1];
  input = fopen(filename, "r");

  ast_node_t *root = parse();
  while (root != NULL) {
    printf("declaration: %s\n", root->s);
    if (root->type == nFUNCTION) {
      ast_node_t *child = root->children;
      if (child->variant == vINT) printf("return type int\n");
      else if (child->variant == vCHAR) printf("return type char\n");
      else printf("return type void\n");
      child = child->next;
      while (child != NULL && child->type == nARGUMENT) {
        printf("argument: %s\n", child->s);
        child = child->next;
      }
    }
    root = root->next;
  }

  return 0;
}
