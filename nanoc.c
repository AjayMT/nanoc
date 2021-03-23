
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
  PLUS_EQ, MINUS_EQ, ASTERISK_EQ, SLASH_EQ, PERCENT_EQ,
  CHAR_LITERAL, STR_LITERAL, IDENT,
  INT, VOID, CHAR, IF, ELSE, WHILE, CONTINUE, BREAK, RETURN
} token_type_t;

typedef struct token_s {
  token_type_t type;
  char *str;
} token_t;

char escape_char(char c)
{
  switch (c) {
  case 'n': return '\n';
  case 't': return '\t';
  }
  return c;
}

token_t next_token() {
  char buf[512]; // max token size is 512
  memset(buf, 0, 512);
  uint32_t buf_idx = 0;
  char c = getc(input);

  while (isspace(c)) c = getc(input); // skip spaces

#define RET_EMPTY_TOKEN(t) return (token_t) { .type = t, .str = NULL }
  switch (c) {
  case EOF: RET_EMPTY_TOKEN(EOF_);
  case ';': RET_EMPTY_TOKEN(SEMICOLON);
  case '(': RET_EMPTY_TOKEN(LPAREN);
  case ')': RET_EMPTY_TOKEN(RPAREN);
  case '{': RET_EMPTY_TOKEN(LCURLY);
  case '}': RET_EMPTY_TOKEN(RCURLY);
  case ',': RET_EMPTY_TOKEN(COMMA);
  case '^': RET_EMPTY_TOKEN(BIT_XOR);
  case '~': RET_EMPTY_TOKEN(BIT_NOT);
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
    ungetc(c, input); RET_EMPTY_TOKEN(BIT_AND);
  }

  if (c == '|') {
    c = getc(input);
    if (c == '|') RET_EMPTY_TOKEN(OR);
    ungetc(c, input); RET_EMPTY_TOKEN(BIT_OR);
  }

  if (c == '+') {
    c = getc(input);
    if (c == '=') RET_EMPTY_TOKEN(PLUS_EQ);
    ungetc(c, input); RET_EMPTY_TOKEN(PLUS);
  }

  if (c == '-') {
    c = getc(input);
    if (c == '=') RET_EMPTY_TOKEN(MINUS_EQ);
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
    return (token_t) { .type = CHAR_LITERAL, .str = s };
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
    return (token_t) { .type = STR_LITERAL, .str = s };
  }

  if (isdigit(c)) {
    while (isdigit(c)) {
      buf[buf_idx] = c; ++buf_idx;
      c = getc(input);
    }
    ungetc(c, input);
    char *s = strndup(buf, buf_idx);
    return (token_t) { .type = INT_LITERAL, .str = s };
  }

  if (isalpha(c)) {
    while (isalnum(c) || c == '_') {
      buf[buf_idx] = c; ++buf_idx;
      c = getc(input);
    }
    ungetc(c, input);

    if (strncmp(buf, "int", 3) == 0) RET_EMPTY_TOKEN(INT);
    if (strncmp(buf, "char", 4) == 0) RET_EMPTY_TOKEN(CHAR);
    if (strncmp(buf, "void", 4) == 0) RET_EMPTY_TOKEN(VOID);
    if (strncmp(buf, "if", 2) == 0) RET_EMPTY_TOKEN(IF);
    if (strncmp(buf, "else", 4) == 0) RET_EMPTY_TOKEN(ELSE);
    if (strncmp(buf, "while", 5) == 0) RET_EMPTY_TOKEN(WHILE);
    if (strncmp(buf, "continue", 8) == 0) RET_EMPTY_TOKEN(CONTINUE);
    if (strncmp(buf, "break", 5) == 0) RET_EMPTY_TOKEN(BREAK);
    if (strncmp(buf, "return", 6) == 0) RET_EMPTY_TOKEN(RETURN);

    char *s = strndup(buf, buf_idx);
    return (token_t) { .type = IDENT, .str = s };
  }

fail:
  printf("Malformed token: %s\nAt position %lu of input\n", buf, ftell(input));
  exit(1);

  return (token_t) { .type = 0, .str = NULL }; // to suppress compiler warning
}

int main(int argc, char *argv[])
{
  if (argc < 2) {
    printf("Usage: nanoc <filename>\n");
    return 1;
  }

  char *filename = argv[1];
  input = fopen(filename, "r");

  while (1) {
    token_t tok = next_token();
    if (tok.str) printf("token: %s\n", tok.str);
    else printf("token type: %d\n", tok.type);
    if (tok.type == EOF_) break;
  }

  return 0;
}
