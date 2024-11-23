#include "poacc.h"

int main(int argc, char **argv) {
  if(argc != 2) {
    fprintf(stderr, "引数の個数が正しくありません\n");
    return 1;
  }

  // tokenize, parseする
  user_input = argv[1];
  token = tokenize();
  Node *node = expr();

  // アセンブリ冒頭部分
  printf(".intel_syntax noprefix\n");
  printf(".globl main\n");
  printf("main:\n");

  // 抽象構文木を下りながらコード生成
  gen(node);

  // スタックトップの式全体の値をloadして返り値にする
  printf("    pop rax\n");
  printf("    ret\n");
  printf(".section	.note.GNU-stack,\"\",@progbits\n");
  return 0;
}
