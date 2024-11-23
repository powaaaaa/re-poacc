#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
******** TOKEN ********
*/

// tokenの種類
typedef enum {
  TK_RESERVED,  // 記号
  TK_NUM,    // 整数トークン
  TK_EOF,    // EOF
} TokenKind;

typedef struct Token Token;

struct Token {
  TokenKind kind;  // tokenの型
  Token *next;    // 次の入力token
  int val;    // kindがTK_NUMの場合の数値
  char *str;    // token文字列
};

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
bool consume(char op){
  if(token->kind != TK_RESERVED || token->str[0] != op) return false;
  token = token->next;
  return true;
}

// 次のtokenが期待している記号のとき, tokenを1つ読み進める. それ以外はerrorを報告する.
void expect(char op) {
  if (token->kind != TK_RESERVED || token->str[0] != op)
    error_at(token->str, "'%c'ではありません", op);
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
Token *new_token(TokenKind kind, Token *cur, char *str) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  cur->next = tok;
  return tok;
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

    if(strchr("+-*/()", *p)) {
      cur = new_token(TK_RESERVED, cur, p++);
      continue;
    }

    if(isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p);
      cur->val = strtol(p, &p, 10);
      continue;
    }

    error_at(p, "不適切なtokenです");
  }

  new_token(TK_EOF, cur, p);
  return head.next;
}

/*
******** PARSER ********
*/

// 抽象構文木のnodeの種類
typedef enum {
  NODE_ADD,    // +
  NODE_SUB,    // -
  NODE_MUL,    // *
  NODE_DIV,    // /
  NODE_NUM,    // 整数
} NodeKind;

typedef struct Node Node;

// 抽象構文木のnodeの型
struct Node {
  NodeKind kind;    // nodeの種類
  Node *lhs;    // 左辺
  Node *rhs;    // 右辺
  int val;    // kindがNODE_NUMのとき使う
};

// 左辺と右辺を受け取るnodeを作成
Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

// 数値を受け取るnodeを作成
Node *new_node_num(int val) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = NODE_NUM;
  node->val = val;
  return node;
}

Node *expr();
Node *mul();
Node *primary();

// parser: `expr = mul ("+" mul | "-" mul)*`
Node *expr() {
  Node *node = mul();

  // 左結合
  for(;;) {
    if(consume('+'))
      node = new_node(NODE_ADD, node, mul());
    else if(consume('-'))
      node = new_node(NODE_SUB, node, mul());
    else
      return node;
  }
}

// parser: `mul = primary ("*" primary | "/" primary)*`
Node *mul() {
  Node *node = primary();

  // 左結合
  for(;;) {
    if(consume('*'))
      node = new_node(NODE_MUL, node, primary());
    else if(consume('/'))
      node = new_node(NODE_DIV, node, primary());
    else
      return node;
  }
}

// parser: `primary = "(" expr ")" | num`
Node *primary() {
  // 次のtokenが"("なら, "(" expr ")"
  if(consume('(')) {
    Node *node = expr();
    expect(')');
    return node;
  }

  // そうでなければ, 数値
  return new_node_num(expect_number());
}

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
  }

  printf("    push rax\n");
}

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
