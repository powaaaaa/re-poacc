#include "poacc.h"

int labelseq = 0;

// Pushes the given node's address to the stack.
void gen_addr(Node *node) {
  if (node->kind == NODE_VAR) {
    printf("    lea rax, [rbp-%d]\n", node->var->offset);
    printf("    push rax\n");
    return;
  }

  error("not an lvalue");
}

void load() {
  printf("    pop rax\n");
  printf("    mov rax, [rax]\n");
  printf("    push rax\n");
}

void store() {
  printf("    pop rdi\n");
  printf("    pop rax\n");
  printf("    mov [rax], rdi\n");
  printf("    push rdi\n");
}

// Generate code for a given node.
void gen(Node *node) {
  switch (node->kind) {
  case NODE_NUM:
    printf("    push %d\n", node->val);
    return;
  case NODE_EXPR_STMT:
    gen(node->lhs);
    printf("    add rsp, 8\n");
    return;
  case NODE_VAR:
    gen_addr(node);
    load();
    return;
  case NODE_ASSIGN:
    gen_addr(node->lhs);
    gen(node->rhs);
    store();
    return;
  case NODE_IF: {
    int seq = labelseq++;
    if (node->els) {
      gen(node->cond);
      printf("    pop rax\n");
      printf("    cmp rax, 0\n");
      printf("    je .Lelse%d\n", seq);
      gen(node->then);
      printf("    jmp .Lend%d\n", seq);
      printf(".Lelse%d:\n", seq);
      gen(node->els);
      printf(".Lend%d:\n", seq);
    } else {
      gen(node->cond);
      printf("    pop rax\n");
      printf("    cmp rax, 0\n");
      printf("    je  .Lend%d\n", seq);
      gen(node->then);
      printf(".Lend%d:\n", seq);
    }
    return;
  }
  case NODE_WHILE: {
    int seq = labelseq++;
    printf(".Lbegin%d:\n", seq);
    gen(node->cond);
    printf("    pop rax\n");
    printf("    cmp rax, 0\n");
    printf("    je  .Lend%d\n", seq);
    gen(node->then);
    printf("    jmp .Lbegin%d\n", seq);
    printf(".Lend%d:\n", seq);
    return;
  }
  case NODE_RETURN:
    gen(node->lhs);
    printf("    pop rax\n");
    printf("    jmp .Lreturn\n");
    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("    pop rdi\n");
  printf("    pop rax\n");

  switch (node->kind) {
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
  }

  printf("    push rax\n");
}

void codegen(Program *prog) {
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  // Prologue
  printf("    push rbp\n");
  printf("    mov rbp, rsp\n");
  printf("    sub rsp, %d\n", prog->stack_size);

  // Emit code
  for (Node *node = prog->node; node; node = node->next)
    gen(node);

  // Epilogue
  printf(".Lreturn:\n");
  printf("    mov rsp, rbp\n");
  printf("    pop rbp\n");
  printf("    ret\n");
  printf(".section	.note.GNU-stack,\"\",@progbits\n");
}
