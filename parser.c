#include <stdlib.h>
#include <string.h>
#include "poacc.h"

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
