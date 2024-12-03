#include "poacc.h"
#include <stdlib.h>
#include <string.h>

VarList *locals;
VarList *globals;

// 変数名からVarを探す
Var *find_var(Token *tok) {
  for (VarList *vl = locals; vl; vl = vl->next) {
    Var *var = vl->var;
    if (strlen(var->name) == tok->len && !memcmp(tok->str, var->name, tok->len))
      return var;
  }

  for (VarList *vl = globals; vl; vl = vl->next) {
    Var *var = vl->var;
    if (strlen(var->name) == tok->len && !memcmp(tok->str, var->name, tok->len))
      return var;
  }
  return NULL;
}

Node *new_node(NodeKind kind, Token *tok) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->tok = tok;
  return node;
}

// 二項演算子のノードを作成
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs, Token *tok) {
  Node *node = new_node(kind, tok);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

// 単項演算子のノードを作成
Node *new_unary(NodeKind kind, Node *expr, Token *tok) {
  Node *node = new_node(kind, tok);
  node->lhs = expr;
  return node;
}

// 整数のノードを作成
Node *new_num(int val, Token *tok) {
  Node *node = new_node(NODE_NUM, tok);
  node->val = val;
  return node;
}

// 変数のノードを作成
Node *new_var(Var *var, Token *tok) {
  Node *node = new_node(NODE_VAR, tok);
  node->var = var;
  return node;
}

// ローカル変数のリストに変数を追加
Var *push_var(char *name, Type *ty, bool is_local) {
  Var *var = calloc(1, sizeof(Var));
  var->name = name;
  var->ty = ty;
  var->is_local = is_local;

  VarList *vl = calloc(1, sizeof(VarList));
  vl->var = var;

  if (is_local) {
    vl->next = locals;
    locals = vl;
  } else {
    vl->next = globals;
    globals = vl;
  }

  return var;
}

Function *function();
Type *basetype();
void global_var();
Node *declaretion();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *postfix();
Node *primary();

bool is_function() {
  Token *tok = token;
  basetype();
  bool isfunc = consume_ident() && consume("(");
  token = tok;
  return isfunc;
}

// `program = (global-var | function)*`
Program *program() {
  Function head;
  head.next = NULL;
  Function *cur = &head;
  globals = NULL;

  while (!at_eof()) {
    if (is_function()) {
      cur->next = function();
      cur = cur->next;
    } else {
      global_var();
    }
  }

  Program *prog = calloc(1, sizeof(Program));
  prog->globals = globals;
  prog->fns = head.next;
  return prog;
}

// basetype = "int" "*"*
Type *basetype() {
  expect("int");
  Type *ty = int_type();
  while (consume("*"))
    ty = pointer_to(ty);
  return ty;
}

Type *read_type_suffix(Type *base) {
  if (!consume("["))
    return base;
  int sz = expect_number();
  expect("]");
  base = read_type_suffix(base);
  return array_of(base, sz);
}

VarList *read_func_param() {
  Type *ty = basetype();
  char *name = expect_ident();
  ty = read_type_suffix(ty);

  VarList *vl = calloc(1, sizeof(VarList));
  vl->var = push_var(name, ty, true);
  return vl;
}

VarList *read_func_params() {
  if (consume(")"))
    return NULL;

  VarList *head = read_func_param();
  VarList *cur = head;

  while (!consume(")")) {
    expect(",");
    cur->next = read_func_param();
    cur = cur->next;
  }

  return head;
}

// `function = basetype ident "(" params? ")" "{" stmt* "}"`
// `params   = ident ("," param)*`
// `param    = basetype ident`
Function *function() {
  locals = NULL;

  Function *fn = calloc(1, sizeof(Function));
  basetype();
  fn->name = expect_ident();
  expect("(");
  fn->params = read_func_params();
  expect("{");
  Node head;
  head.next = NULL;
  Node *cur = &head;

  while (!consume("}")) {
    cur->next = stmt();
    cur = cur->next;
  }

  fn->node = head.next;
  fn->locals = locals;
  return fn;
}

// global-var = basetype ident ("[" num "]")* ";"
void global_var() {
  Type *ty = basetype();
  char *name = expect_ident();
  ty = read_type_suffix(ty);
  expect(";");
  push_var(name, ty, false);
}

// `declaretion = basetype ident ("[" num "]")* ("=" expr) ";"`
Node *declaretion() {
  Token *tok = token;
  Type *ty = basetype();
  char *name = expect_ident();
  ty = read_type_suffix(ty);
  Var *var = push_var(name, ty, true);

  if (consume(";"))
    return new_node(NODE_NULL, tok);

  expect("=");
  Node *lhs = new_var(var, tok);
  Node *rhs = expr();
  expect(";");
  Node *node = new_binary(NODE_ASSIGN, lhs, rhs, tok);

  return new_unary(NODE_EXPR_STMT, node, tok);
}

Node *read_expr_stmt() {
  Token *tok = token;
  return new_unary(NODE_EXPR_STMT, expr(), tok);
}

// `stmt = "return" expr ";"
//        | "{" stmt* "}"
//        | "if" "(" expr ")" stmt ("else" stmt)?
//        | "while" "(" expr ")" stmt
//        | "for" "(" expr? ";" expr? ";" expr?")"
//        | declaretion
//        | expr ";"`
Node *stmt() {
  Token *tok;
  if (tok = consume("return")) {
    Node *node = new_unary(NODE_RETURN, expr(), tok);
    expect(";");
    return node;
  }

  if (tok = consume("if")) {
    Node *node = new_node(NODE_IF, tok);
    expect("(");
    node->cond = expr();
    expect(")");
    node->then = stmt();
    if (consume("else"))
      node->els = stmt();
    return node;
  }

  if (tok = consume("while")) {
    Node *node = new_node(NODE_WHILE, tok);
    expect("(");
    node->cond = expr();
    expect(")");
    node->then = stmt();
    return node;
  }

  if (tok = consume("for")) {
    Node *node = new_node(NODE_FOR, tok);
    expect("(");
    if (!consume(";")) {
      node->init = read_expr_stmt();
      expect(";");
    }
    if (!consume(";")) {
      node->cond = expr();
      expect(";");
    }
    if (!consume(")")) {
      node->inc = read_expr_stmt();
      expect(")");
    }
    node->then = stmt();
    return node;
  }

  if (tok = consume("{")) {
    Node head;
    head.next = NULL;
    Node *cur = &head;

    while (!consume("}")) {
      cur->next = stmt();
      cur = cur->next;
    }

    Node *node = new_node(NODE_BLOCK, tok);
    node->body = head.next;
    return node;
  }

  if (tok = peek("int"))
    return declaretion();

  Node *node = read_expr_stmt();
  expect(";");
  return node;
}

// `expr = assign`
Node *expr() { return assign(); }

// `assign = equality ("=" assign)?`
Node *assign() {
  Node *node = equality();
  Token *tok;
  if (tok = consume("="))
    node = new_binary(NODE_ASSIGN, node, assign(), tok);
  return node;
}

// `equality = relational ("==" relational | "!=" relational)*`
Node *equality() {
  Node *node = relational();
  Token *tok;

  for (;;) {
    if (tok = consume("=="))
      node = new_binary(NODE_EQ, node, relational(), tok);
    else if (tok = consume("!="))
      node = new_binary(NODE_NE, node, relational(), tok);
    else
      return node;
  }
}

// `relational = add ("<" add | "<=" add | ">" add | ">=" add)*`
Node *relational() {
  Node *node = add();
  Token *tok;

  for (;;) {
    if (tok = consume("<"))
      node = new_binary(NODE_LT, node, add(), tok);
    else if (tok = consume("<="))
      node = new_binary(NODE_LE, node, add(), tok);
    else if (tok = consume(">"))
      node = new_binary(NODE_LT, add(), node, tok);
    else if (tok = consume(">="))
      node = new_binary(NODE_LE, add(), node, tok);
    else
      return node;
  }
}

// `add = mul ("+" mul | "-" mul)*`
Node *add() {
  Node *node = mul();
  Token *tok;

  for (;;) {
    if (tok = consume("+"))
      node = new_binary(NODE_ADD, node, mul(), tok);
    else if (tok = consume("-"))
      node = new_binary(NODE_SUB, node, mul(), tok);
    else
      return node;
  }
}

// `mul = unary ("*" unary | "/" unary)*`
Node *mul() {
  Node *node = unary();
  Token *tok;

  for (;;) {
    if (tok = consume("*"))
      node = new_binary(NODE_MUL, node, unary(), tok);
    else if (tok = consume("/"))
      node = new_binary(NODE_DIV, node, unary(), tok);
    else
      return node;
  }
}

// `unary = ("+" | "-" | "*" | "&" )? unary
//          | postfix`
Node *unary() {
  Token *tok;
  if (tok = consume("+"))
    return unary();
  if (tok = consume("-"))
    return new_binary(NODE_SUB, new_num(0, tok), unary(), tok);
  if (tok = consume("&"))
    return new_unary(NODE_ADDR, unary(), tok);
  if (tok = consume("*"))
    return new_unary(NODE_DEREF, unary(), tok);
  return postfix();
}

// postfix = primary ("[" expr "]")*
Node *postfix() {
  Node *node = primary();
  Token *tok;

  while (tok = consume("[")) {
    // x[y] is short for *(x+y)
    Node *exp = new_binary(NODE_ADD, node, expr(), tok);
    expect("]");
    node = new_unary(NODE_DEREF, exp, tok);
  }
  return node;
}

// func-args = "(" assign ("," assign)*? ")"
Node *func_args() {
  if (consume(")"))
    return NULL;

  Node *head = assign();
  Node *cur = head;
  while (consume(",")) {
    cur->next = assign();
    cur = cur->next;
  }

  expect(")");
  return head;
}

// `primary = "(" expr ")"
//            | "sizeof" unary
//            | ident args?
//            | num`
// `args = "(" ")"`
Node *primary() {
  if (consume("(")) {
    Node *node = expr();
    expect(")");
    return node;
  }

  Token *tok;
  if (tok = consume("sizeof"))
    return new_unary(NODE_SIZEOF, unary(), tok);

  if (tok = consume_ident()) {
    if (consume("(")) {
      Node *node = new_node(NODE_FUNCALL, tok);
      node->funcname = strndupl(tok->str, tok->len);
      node->args = func_args();
      return node;
    }
    Var *var = find_var(tok);
    if (!var)
      error_tok(tok, "undefined variable");
    return new_var(var, tok);
  }

  tok = token;
  if (tok->kind != TK_NUM)
    error_tok(tok, "expected expression");
  return new_num(expect_number(), tok);
}
