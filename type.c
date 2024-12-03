#include "poacc.h"

#include <stdlib.h>

Type *int_type() {
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = TY_INT;
  return ty;
}
Type *pointer_to(Type *base) {
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = TY_PTR;
  ty->base = base;
  return ty;
}
void visit(Node *node) {
  if (!node)
    return;
  visit(node->lhs);
  visit(node->rhs);
  visit(node->cond);
  visit(node->then);
  visit(node->els);
  visit(node->init);
  visit(node->inc);
  for (Node *n = node->body; n; n = n->next)
    visit(n);
  for (Node *n = node->args; n; n = n->next)
    visit(n);
  switch (node->kind) {
  case NODE_MUL:
  case NODE_DIV:
  case NODE_EQ:
  case NODE_NE:
  case NODE_LT:
  case NODE_LE:
  case NODE_FUNCALL:
  case NODE_NUM:
    node->ty = int_type();
    return;
  case NODE_VAR:
    node->ty = node->var->ty;
    return;
  case NODE_ADD:
    if (node->rhs->ty->kind == TY_PTR) {
      Node *tmp = node->lhs;
      node->lhs = node->rhs;
      node->rhs = tmp;
    }
    if (node->rhs->ty->kind == TY_PTR)
      error_tok(node->tok, "invalid pointer arithmetic operands");
    node->ty = node->lhs->ty;
    return;
  case NODE_SUB:
    if (node->rhs->ty->kind == TY_PTR)
      error_tok(node->tok, "invalid pointer arithmetic operands");
    node->ty = node->lhs->ty;
    return;
  case NODE_ASSIGN:
    node->ty = node->lhs->ty;
    return;
  case NODE_ADDR:
    node->ty = pointer_to(node->lhs->ty);
    return;
  case NODE_DEREF:
    if (node->lhs->ty->kind != TY_PTR)
      error_tok(node->tok, "invalid pointer dereference");
    node->ty = node->lhs->ty->base;
    return;
  }
}
void add_type(Function *prog) {
  for (Function *fn = prog; fn; fn = fn->next)
    for (Node *node = fn->node; node; node = node->next)
      visit(node);
}
