
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
    if ((*current_arg)->variant != vBLOCK && (*current_arg)->variant != vEMPTY)
      goto fail;

    current = &((*current)->next);
    tok = next_token();
  }

  return root;
}

uint32_t hash(char *str)
{
  uint32_t h = 5381;
  int c;

  while ((c = *str++))
    h = (h << 5) + h + c;

  return h;
}

typedef enum {
  tINT, tCHAR, tVOID, tINT_PTR, tCHAR_PTR, tVOID_PTR, tPTR_PTR
} symbol_type_t;

typedef enum {
  lTEXT, lDATA, lSTACK
} loc_type_t;

typedef struct symbol_s {
  char *name;
  symbol_type_t type;
  uint32_t loc;
  loc_type_t loc_type;
  struct symbol_s *child;
  struct symbol_s *parent;
} symbol_t;

uint32_t *text = NULL;
uint32_t text_loc = 0;
uint32_t text_cap = 0;
uint32_t *data = NULL;
uint32_t data_loc = 0;
uint32_t data_cap = 0;

// this is arbitrary
#define SYMTAB_SIZE 256

symbol_t *root_symtab = NULL;
uint32_t block_id = 0;

void symtab_insert(symbol_t *tab, symbol_t sym)
{
  uint32_t idx = hash(sym.name) % SYMTAB_SIZE;
  uint32_t i = idx;
  do {
    if (tab[i].name == NULL) {
      sym.parent = tab;
      tab[i] = sym;
      return;
    }
    if (strcmp(tab[i].name, sym.name) == 0) {
      printf("Duplicate symbol: %s\n", sym.name);
      exit(1);
    }
    ++i;
  } while (i != idx);
  printf("Too many (>256) symbols\n");
  exit(1);
}

symbol_type_t symbol_type_of_node_type(ast_node_t *v)
{
  switch (v->variant) {
  case vINT: return tINT;
  case vCHAR: return tCHAR;
  case vVOID: return tVOID;
  case vPTR:
    switch (v->children->variant) {
    case vINT: return tINT_PTR;
    case vCHAR: return tCHAR_PTR;
    case vVOID: return tVOID_PTR;
    case vPTR: return tPTR_PTR;
    }
  }

  return 0;
}

uint32_t construct_symtab(ast_node_t *root, symbol_t *out, uint32_t loc)
{
  if (root->type != nSTMT && root->type != nFUNCTION) {
    printf("Failed to construct symbol table\n");
    exit(1);
  }

  if (root->type == nFUNCTION) {
    symbol_t sym;
    memset(&sym, 0, sizeof(symbol_t));
    sym.name = root->s;
    ast_node_t *type_node = root->children;
    sym.type = symbol_type_of_node_type(type_node);
    sym.loc = text_loc;
    sym.loc_type = lTEXT;
    sym.child = malloc(sizeof(symbol_t) * SYMTAB_SIZE);
    memset(sym.child, 0, sizeof(symbol_t) * SYMTAB_SIZE);

    ast_node_t *argument_nodes = type_node->next;
    ast_node_t *current_arg = argument_nodes;
    uint32_t arg_offset = 8; // 4 bytes for return address and 4 for old ebp
    while (current_arg->type == nARGUMENT) {
      symbol_t arg_sym;
      arg_sym.name = current_arg->s;
      arg_sym.loc = arg_offset;
      arg_sym.loc_type = lSTACK;
      arg_sym.type = symbol_type_of_node_type(current_arg->children);
      arg_sym.child = NULL;
      symtab_insert(sym.child, arg_sym);
      if (arg_sym.type == tCHAR) arg_offset += 1;
      else arg_offset += 4;
      current_arg = current_arg->next;
    }

    uint32_t stack_size = construct_symtab(current_arg, sym.child, 0);
    symtab_insert(out, sym);
    return stack_size;
  }

  if (root->variant == vDECL) {
    symbol_t sym;
    sym.name = root->s;
    if (out == root_symtab) {
      sym.loc = data_loc;
      sym.loc_type = lDATA;
    } else {
      sym.loc = loc;
      sym.loc_type = lSTACK;
    }
    sym.type = symbol_type_of_node_type(root->children);
    sym.child = NULL;
    symtab_insert(out, sym);
    if (sym.type == tCHAR) return 1;
    return 4;
  }

  if (root->variant == vBLOCK) {
    if (root->children == NULL) return 0;

    symbol_t sym;
    sym.name = malloc(2);
    sym.name[0] = block_id % 256; ++block_id;
    sym.name[1] = 0;
    sym.child = malloc(sizeof(symbol_t) * SYMTAB_SIZE);
    sym.loc = loc;
    sym.loc_type = lSTACK;

    ast_node_t *current_child = root->children;
    uint32_t size = 0;
    while (current_child != NULL) {
      size += construct_symtab(current_child, sym.child, loc + size);
      current_child = current_child->next;
    }

    symtab_insert(out, sym);
    return size;
  }

  if (root->variant == vWHILE)
    return construct_symtab(root->children->next, out, loc);

  if (root->variant == vIF) {
    uint32_t s = construct_symtab(root->children->next, out, loc);
    return construct_symtab(root->children->next->next, out, loc + s);
  }

  return 0;
}

void codegen(ast_node_t *ast)
{
  // TODO
  construct_symtab(ast, root_symtab, 0);
}

int main(int argc, char *argv[])
{
  if (argc < 2) {
    printf("Usage: nanoc <filename>\n");
    return 1;
  }

  char *filename = argv[1];
  input = fopen(filename, "r");

  root_symtab = malloc(sizeof(symbol_t) * SYMTAB_SIZE);
  memset(root_symtab, 0, sizeof(symbol_t) * SYMTAB_SIZE);

  ast_node_t *root = parse();
  codegen(root);

  // testing code
  for (uint32_t i = 0; i < SYMTAB_SIZE; ++i) {
    if (root_symtab[i].name == NULL) continue;

    printf("symbol %s\n", root_symtab[i].name);
    if (root_symtab[i].child != NULL) {
      symbol_t *child = NULL;
      for (uint32_t k = 0; k < SYMTAB_SIZE; ++k)
        if (root_symtab[i].child[k].child != NULL)
          child = root_symtab[i].child[k].child;
      for (uint32_t j = 0; j < SYMTAB_SIZE; ++j) {
        if (child[j].name == NULL) continue;

        printf("child symbol %s loc %d\n", child[j].name, child[j].loc);
      }
    }
  }

  return 0;
}
