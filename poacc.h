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
void expect(char *op);
int expect_number();
bool at_eof();
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
bool startswith(char *p, char *q);
Token *tokenize();

/*
******** PARSER ********
*/

// 抽象構文木のnodeの種類
typedef enum {
  NODE_ADD,    // +
  NODE_SUB,    // -
  NODE_MUL,    // *
  NODE_DIV,    // /
  NODE_EQ,    // ==
  NODE_NE,    // !=
  NODE_LT,    // <
  NODE_LE,    // <=
  NODE_NUM,    // 整数
} NodeKind;

// 抽象構文木のnodeの型
typedef struct Node Node;
struct Node {
  NodeKind kind;    // nodeの種類
  Node *lhs;    // 左辺
  Node *rhs;    // 右辺
  int val;    // kindがNODE_NUMのとき使う
};

Node *new_node(NodeKind kind);
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);

Node *expr();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

/*
******** CODE GENERATOR ********
*/

void gen(Node *node);

#endif // POACC_H
