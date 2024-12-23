#include "poacc.h"

#include <stdlib.h>

Type *new_type(TypeKind kind) {
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = kind;
  return ty;
}

Type *char_type() { return new_type(TY_CHAR); }

Type *int_type() { return new_type(TY_INT); }

Type *pointer_to(Type *base) {
  Type *ty = new_type(TY_PTR);
  ty->base = base;
  return ty;
}

Type *array_of(Type *base, int size) {
  Type *ty = new_type(TY_ARRAY);
  ty->base = base;
  ty->array_size = size;
  return ty;
}

int size_of(Type *ty) {
  switch (ty->kind) {
  case TY_CHAR:
    return 1;
  case TY_INT:
  case TY_PTR:
    return 8;
  default:
    assert(ty->kind == TY_ARRAY);
    return size_of(ty->base) * ty->array_size;
  }
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
    if (node->rhs->ty->base) {
      Node *tmp = node->lhs;
      node->lhs = node->rhs;
      node->rhs = tmp;
    }
    if (node->rhs->ty->base)
      error_tok(node->tok, "invalid pointer arithmetic operands");
    node->ty = node->lhs->ty;
    return;
  case NODE_SUB:
    if (node->rhs->ty->base)
      error_tok(node->tok, "invalid pointer arithmetic operands");
    node->ty = node->lhs->ty;
    return;
  case NODE_ASSIGN:
    node->ty = node->lhs->ty;
    return;
  case NODE_ADDR:
    if (node->lhs->ty->kind == TY_ARRAY)
      node->ty = pointer_to(node->lhs->ty->base);
    else
      node->ty = pointer_to(node->lhs->ty);
    return;
  case NODE_DEREF:
    if (!node->lhs->ty->base)
      error_tok(node->tok, "invalid pointer dereference");
    node->ty = node->lhs->ty->base;
    return;
  case NODE_SIZEOF:
    node->kind = NODE_NUM;
    node->ty = int_type();
    node->val = size_of(node->lhs->ty);
    node->lhs = NULL;
    return;
  case NODE_STMT_EXPR: {
    Node *last = node->body;
    while (last->next)
      last = last->next;
    node->ty = last->ty;
    return;
  }
  }
}
void add_type(Program *prog) {
  for (Function *fn = prog->fns; fn; fn = fn->next)
    for (Node *node = fn->node; node; node = node->next)
      visit(node);
}
