#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "poacc.h"

/*
******** TOKEN ********
*/

// 現在注目してるtoken
Token *token;

// 入力プログラム
char *user_input;

// errorを報告する関数
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// error箇所を報告する
void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, " ");    // pos個の空白を出力
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// 次のtokenが期待している記号のとき, tokenを1つ読み進めて真を返す. それ以外は偽を返す.
bool consume(char *op){
  if(token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len))
    return false;
  token = token->next;
  return true;
}

// 次のtokenが期待している記号のとき, tokenを1つ読み進める. それ以外はerrorを報告する.
void expect(char *op) {
  if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len))
    error_at(token->str, "'%s'ではありません", op);
  token = token->next;
}

// 次のtokenが数値の場合, tokenを1つ読み進めてその数値を返す. それ以外はerrorを報告する.
int expect_number() {
  if(token->kind != TK_NUM) error_at(token->str, "数ではありません");
  int val = token->val;
  token = token->next;
  return val;
}

// eofかどうか
bool at_eof() {
  return token->kind == TK_EOF;
}

// 新しいtokenを作成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

bool startswith(char *p, char *q) {
  return memcmp(p, q, strlen(q)) == 0;
}

// 入力文字列pをtokenizeして返す
Token *tokenize() {
  char *p = user_input;
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while(*p) {
    // 空白をスキップ
    if(isspace(*p)) {
      p++;
      continue;
    }

    // 複数文字
    if(startswith(p, "==") || startswith(p, "!=") || startswith(p, "<=") || startswith(p, ">=")) {
      cur = new_token(TK_RESERVED, cur, p, 2);
      p += 2;
      continue;
    }

    // 1文字
    if(strchr("+-*/()<>", *p)) {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }

    // 数値
    if(isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p, 0);
      char *q = p;
      cur->val = strtol(p, &p, 10);
      cur->len = p - q;
      continue;
    }

    error_at(p, "不適切なtokenです");
  }

  new_token(TK_EOF, cur, p, 0);
  return head.next;
}

/*
******** PARSER ********
*/

// 左辺と右辺を受け取るnodeを作成
Node *new_node(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

// 左辺と右辺を受け取るnodeを作成
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

// 数値を受け取るnodeを作成
Node *new_node_num(int val) {
  Node *node = new_node(NODE_NUM);
  node->val = val;
  return node;
}

// parser: `expr = equality`
Node *expr() {
  return equality();
}

// parser: `equality = relational ("==" relational | "!=" relational)*`
Node *equality() {
  Node *node = relational();

  for(;;) {
    if(consume("=="))
      node = new_binary(NODE_EQ, node, relational());
    else if(consume("!="))
        node = new_binary(NODE_NE, node, relational());
    else
      return node;
  }
}

// parser: `relational = add ("<" add | "<=" add | ">" add | ">=" add)*`
Node *relational() {
  Node *node = add();

  for(;;) {
    if(consume("<"))
      node = new_binary(NODE_LT, node, add());
    else if(consume("<="))
      node = new_binary(NODE_LE, node, add());
    else if(consume(">"))
      node = new_binary(NODE_LT, add(), node);
    else if(consume(">="))
      node = new_binary(NODE_LE, add(), node);
    else
      return node;
  }
}

// parser: `add = mul ("+" mul | "-" mul)*`
Node *add() {
  Node *node = mul();

  for(;;){
    if(consume("+"))
      node = new_binary(NODE_ADD, node, mul());
    else if(consume("-"))
      node = new_binary(NODE_SUB, node, mul());
    else
      return node;
  }
}

// parser: `mul = unary ("*" unary | "/" unary)*`
Node *mul() {
  Node *node = unary();

  // 左結合
  for(;;) {
    if(consume("*"))
      node = new_binary(NODE_MUL, node, unary());
    else if(consume("/"))
      node = new_binary(NODE_DIV, node, unary());
    else
      return node;
  }
}

// parser: `unary - ("+" | "-")? unary | primary`
Node *unary() {
  if(consume("+"))
    return unary();
  if(consume("-"))
    return new_binary(NODE_SUB, new_node_num(0), unary());
  return primary();
}

// parser: `primary = "num | (" expr ")"`
Node *primary() {
  // 次のtokenが"("なら, "(" expr ")"
  if(consume("(")) {
    Node *node = expr();
    expect(")");
    return node;
  }

  // そうでなければ, 数値
  return new_node_num(expect_number());
}
