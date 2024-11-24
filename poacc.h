#ifndef POACC_H
#define POACC_H

#include <stdbool.h>
#include <stdio.h>

/*
******** TOKEN ********
*/

// tokenの種類
typedef enum {
  TK_RESERVED,  // 記号
  TK_IDENT,    // 識別子
  TK_NUM,    // 整数トークン
  TK_EOF,    // EOF
} TokenKind;

// tokenの型
typedef struct Token Token;
struct Token {
  TokenKind kind;  // tokenの型
  Token *next;    // 次の入力token
  int val;    // kindがTK_NUMの場合の数値
  char *str;    // token文字列
  int len;    // tokenの長さ
};

// 現在注目してるtoken
extern Token *token;

// 入力プログラム
extern char *user_input;

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
bool consume(char *op);
Token *consume_ident();
char *strndupl(char *p, int len);
void expect(char *op);
int expect_number();
bool at_eof();
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
bool startswith(char *p, char *q);
Token *tokenize();

/*
******** PARSER ********
*/

// ローカル変数
typedef struct Var Var;
struct Var {
  Var *next;
  char *name;    // 変数名
  int offset;    // RBPからのオフセット
};

// 抽象構文木
typedef enum {
  NODE_ADD,    // +
  NODE_SUB,    // -
  NODE_MUL,    // *
  NODE_DIV,    // /
  NODE_EQ,    // ==
  NODE_NE,    // !=
  NODE_LT,    // <
  NODE_LE,    // <=
  NODE_ASSIGN,    // =
  NODE_RETURN,    // "return"
  NODE_EXPR_STMT, // Expression statement
  NODE_VAR,    // 変数
  NODE_NUM,    // 整数
} NodeKind;

// nodeの型
typedef struct Node Node;
struct Node {
  NodeKind kind;    // nodeの種類
  Node *next;    // 次のnode
  Node *lhs;    // 左辺
  Node *rhs;    // 右辺
  Var *var;    // NODE_VARのとき使う
  int val;    // NODE_NUMのとき使う
};

// programの型
typedef struct {
  Node *node;
  Var *locals;
  int stack_size;
} Program;

// 複数のnodeを保存
// extern Node *code[100];

Program *program();

/*
******** CODE GENERATOR ********
*/

void codegen(Program *prog);

#endif // POACC_H
