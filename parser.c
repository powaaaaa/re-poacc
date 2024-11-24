#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "poacc.h"

/*
******** TOKEN ********
*/

// 入力
char *user_input;
// 現在のtoken
Token *token;

// errorを報告
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// エラー箇所を報告
void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, ""); // print pos spaces.
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

char *strndupl(char *p, int len) {
  char *buf = malloc(len + 1);
  strncpy(buf, p, len);
  buf[len] = '\0';
  return buf;
}

// 現在のtokenが`op`のとき, tokenを1つ進める
bool consume(char *op) {
  if (token->kind != TK_RESERVED || strlen(op) != token->len ||
      memcmp(token->str, op, token->len))
    return false;
  token = token->next;
  return true;
}

// 現在のtokenが識別子のとき, tokenを1つ進めてそのtokenを返す
Token *consume_ident() {
  if (token->kind != TK_IDENT)
    return NULL;
  Token *t = token;
  token = token->next;
  return t;
}

// 現在のtokenが`op`であることを確認, tokenを1つ進める
void expect(char *op) {
  if (token->kind != TK_RESERVED || strlen(op) != token->len ||
      memcmp(token->str, op, token->len))
    error_at(token->str, "expected \"%s\"", op);
  token = token->next;
}

// 現在のtokenがTK_NUMであることを確認, tokenを進めてその値を返す
int expect_number() {
  if (token->kind != TK_NUM)
    error_at(token->str, "expected a number");
  int val = token->val;
  token = token->next;
  return val;
}

// 現在のtokenがEOFであるかどうか
bool at_eof() {
  return token->kind == TK_EOF;
}

// 新しいtokenを作成し, それを`cur`の次のtokenとして追加する
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

// `p`が`q`で始まるかどうか
bool startswith(char *p, char *q) {
  return memcmp(p, q, strlen(q)) == 0;
}

// `c`がアルファベットかアンダースコアかどうか
bool is_alpha(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

// `c`がアルファベットか数字かどうか
bool is_alnum(char c) {
  return is_alpha(c) || ('0' <= c && c <= '9');
}

// `userInput`をトークン分割して新しいtokenを返す
Token *tokenize() {
  char *p = user_input;
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p) {
    // 空白をスキップ
    if (isspace(*p)) {
      p++;
      continue;
    }

    // Keyword
    if (startswith(p, "return") && !is_alnum(p[6])) {
      cur = new_token(TK_RESERVED, cur, p, 6);
      p += 6;
      continue;
    }

    // 複数文字
    if (startswith(p, "==") || startswith(p, "!=") ||
        startswith(p, "<=") || startswith(p, ">=")) {
      cur = new_token(TK_RESERVED, cur, p, 2);
      p += 2;
      continue;
    }

    // 1文字
    if (strchr("+-*/()<>;=", *p)) {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }

    // 識別子
    if (is_alpha(*p)) {
      char *q = p++;
      while (is_alnum(*p))
        p++;
      cur = new_token(TK_IDENT, cur, q, p - q);
      continue;
    }

    // 整数リテラル
    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p, 0);
      char *q = p;
      cur->val = strtol(p, &p, 10);
      cur->len = p - q;
      continue;
    }

    error_at(p, "invalid token");
  }

  new_token(TK_EOF, cur, p, 0);
  return head.next;
}

/*
******** PARSER ********
*/

Var *locals;

// 変数名からVarを探す
Var *find_var(Token *tok) {
  for (Var *var = locals; var; var = var->next)
    if (strlen(var->name) == tok->len && !memcmp(tok->str, var->name, tok->len))
      return var;
  return NULL;
}

Node *new_node(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

// 二項演算子のノードを作成
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

// 単項演算子のノードを作成
Node *new_unary(NodeKind kind, Node *expr) {
  Node *node = new_node(kind);
  node->lhs = expr;
  return node;
}

// 整数のノードを作成
Node *new_num(int val) {
  Node *node = new_node(NODE_NUM);
  node->val = val;
  return node;
}

// 変数のノードを作成
Node *new_var(Var *var) {
  Node *node = new_node(NODE_VAR);
  node->var = var;
  return node;
}

// ローカル変数のリストに変数を追加
Var *push_var(char *name) {
  Var *var = calloc(1, sizeof(Var));
  var->next = locals;
  var->name = name;
  locals = var;
  return var;
}

Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

// `program = stmt*`
Program *program() {
  locals = NULL;

  Node head;
  head.next = NULL;
  Node *cur = &head;

  while (!at_eof()) {
    cur->next = stmt();
    cur = cur->next;
  }

  Program *prog = calloc(1, sizeof(Program));
  prog->node = head.next;
  prog->locals = locals;
  return prog;
}

// `stmt = "return" expr ";" | expr ";"`
Node *stmt() {
  if (consume("return")) {
    Node *node = new_unary(NODE_RETURN, expr());
    expect(";");
    return node;
  }

  Node *node = new_unary(NODE_EXPR_STMT, expr());
  expect(";");
  return node;
}

// `expr = assign`
Node *expr() {
  return assign();
}

// `assign = equality ("=" assign)?`
Node *assign() {
  Node *node = equality();
  if (consume("="))
    node = new_binary(NODE_ASSIGN, node, assign());
  return node;
}

// `equality = relational ("==" relational | "!=" relational)*`
Node *equality() {
  Node *node = relational();

  for (;;) {
    if (consume("=="))
      node = new_binary(NODE_EQ, node, relational());
    else if (consume("!="))
      node = new_binary(NODE_NE, node, relational());
    else
      return node;
  }
}

// `relational = add ("<" add | "<=" add | ">" add | ">=" add)*`
Node *relational() {
  Node *node = add();

  for (;;) {
    if (consume("<"))
      node = new_binary(NODE_LT, node, add());
    else if (consume("<="))
      node = new_binary(NODE_LE, node, add());
    else if (consume(">"))
      node = new_binary(NODE_LT, add(), node);
    else if (consume(">="))
      node = new_binary(NODE_LE, add(), node);
    else
      return node;
  }
}

// `add = mul ("+" mul | "-" mul)*`
Node *add() {
  Node *node = mul();

  for (;;) {
    if (consume("+"))
      node = new_binary(NODE_ADD, node, mul());
    else if (consume("-"))
      node = new_binary(NODE_SUB, node, mul());
    else
      return node;
  }
}

// `mul = unary ("*" unary | "/" unary)*`
Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume("*"))
      node = new_binary(NODE_MUL, node, unary());
    else if (consume("/"))
      node = new_binary(NODE_DIV, node, unary());
    else
      return node;
  }
}

// `unary = ("+" | "-")? unary | primary`
Node *unary() {
  if (consume("+"))
    return unary();
  if (consume("-"))
    return new_binary(NODE_SUB, new_num(0), unary());
  return primary();
}

// `primary = "(" expr ")" | ident | num`
Node *primary() {
  if (consume("(")) {
    Node *node = expr();
    expect(")");
    return node;
  }

  Token *tok = consume_ident();
  if (tok) {
    Var *var = find_var(tok);
    if (!var)
      var = push_var(strndupl(tok->str, tok->len));
    return new_var(var);
  }

  return new_num(expect_number());
}
