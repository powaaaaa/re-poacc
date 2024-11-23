#include "poacc.h"

/*
******** CODE GENERATOR ********
*/

void gen(Node *node) {
  if(node->kind == NODE_NUM) {
    printf("    push %d\n", node->val);
    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("    pop rdi\n");
  printf("    pop rax\n");

  switch(node->kind) {
    case NODE_ADD:
      printf("    add rax, rdi\n");
      break;
    case NODE_SUB:
      printf("    sub rax, rdi\n");
      break;
    case NODE_MUL:
      printf("    imul rax, rdi\n");
      break;
    case NODE_DIV:
      printf("    cqo\n");
      printf("    idiv rdi\n");
      break;
    case NODE_EQ:
      printf("    cmp rax, rdi\n");
      printf("    sete al\n");
      printf("    movzb rax, al\n");
      break;
    case NODE_NE:
      printf("    cmp rax, rdi\n");
      printf("    setne al\n");
      printf("    movzb rax, al\n");
      break;
    case NODE_LT:
      printf("    cmp rax, rdi\n");
      printf("    setl al\n");
      printf("    movzb rax, al\n");
      break;
    case NODE_LE:
      printf("    cmp rax, rdi\n");
      printf("    setle al\n");
      printf("    movzb rax, al\n");
      break;
    case NODE_NUM:
      break;
  }

  printf("    push rax\n");
}
