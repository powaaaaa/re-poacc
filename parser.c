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

char *strndupl(char *p, int len) {
  char *buf = malloc(len + 1);
  strncpy(buf, p, len);
  buf[len] = '\0';
  return buf;
}

// 次のtokenが期待している記号のとき, tokenを1つ読み進めて真を返す. それ以外は偽を返す.
bool consume(char *op){
  if(token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len))
    return false;
  token = token->next;
  return true;
}

Token *consume_ident() {
  if(token->kind != TK_IDENT)
    return NULL;
  Token *t = token;
  token = token->next;
  return t;
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

bool is_alpha(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

bool is_alnum(char c) {
  return is_alpha(c) || ('0' <= c && c <= '9');
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

    // keyword
    if(startswith(p, "return") && !is_alnum(p[6])) {
      cur = new_token(TK_RESERVED, cur, p, 6);
      p += 6;
      continue;
    }

    // 複数文字
    if(startswith(p, "==") || startswith(p, "!=") || startswith(p, "<=") || startswith(p, ">=")) {
      cur = new_token(TK_RESERVED, cur, p, 2);
      p += 2;
      continue;
    }

    // 1文字
    if(strchr("+-*/()<>;=", *p)) {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }

    // 識別子
    if(is_alpha(*p)) {
      char *q = p++;
      while(is_alnum(*p))
        p++;

      cur = new_token(TK_IDENT, cur, q, p - q);
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

// Node *code[100];
Var *locals;

// 変数名からローカル変数を見つける
Var *find_var(Token *tok) {
  for(Var *var = locals; var; var = var->next) {
    if(strlen(var->name) == tok->len && !memcmp(tok->str, var->name, tok->len))
      return var;
  }
  return NULL;
}

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

// 変数を受け取るnodeを作成
Node *new_var(Var *var) {
  Node *node = new_node(NODE_VAR);
  node->var = var;
  return node;
}

// 数値を受け取るnodeを作成
Node *new_node_num(int val) {
  Node *node = new_node(NODE_NUM);
  node->val = val;
  return node;
}

Node *new_unary(NodeKind kind, Node *expr) {
  Node *node = new_node(kind);
  node->lhs = expr;
  return node;
}

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

// parser: `program = stmt*`
Program *program() {
  locals = NULL;

  Node head;
  head.next = NULL;
  Node *cur = &head;

  while(!at_eof()) {
    cur->next = stmt();
    cur = cur->next;
  }

  Program *prog = calloc(1, sizeof(Program));
  prog->node = head.next;
  prog->locals = locals;
  return prog;
}

// parser: `stmt = expr ";"`
Node *stmt() {
  Node *node = new_unary(NODE_EXPR_STMT, expr());
  // expect(";");
  return node;
}

// parser: `expr = assign`
Node *expr() {
  return assign();
}

// parser: `assign = equality ("=" assign)?`
Node *assign() {
  Node *node = equality();
  if(consume("="))
    node = new_binary(NODE_ASSIGN, node, assign());
  return node;
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

// parser: `primary = "num | ident | (" expr ")"`
Node *primary() {
  // 次のtokenが"("なら, "(" expr ")"
  if(consume("(")) {
    Node *node = expr();
    expect(")");
    return node;
  }

  Token *tok = consume_ident();
  if(tok) {
    Var *var = find_var(tok);
    if(!var)
      var = push_var(strndupl(tok->str, tok->len));
    return new_var(var);
  }

  // そうでなければ, 数値
  return new_node_num(expect_number());
}
