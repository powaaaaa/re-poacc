#ifndef POACC_H
#define POACC_H

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct Type Type;

/*
******** TOKEN ********
*/

// Token
typedef enum {
  TK_RESERVED, // Keywords or punctuators
  TK_IDENT,    // 識別子
  TK_NUM,      // 整数リテラル
  TK_EOF,      // EOFマーカー
} TokenKind;

// Token type
typedef struct Token Token;
struct Token {
  TokenKind kind; // tokenの種類
  Token *next;    // 次のtoken
  int val;        // TK_NUM時の値
  char *str;      // tokenの文字列
  int len;        // tokenの長さ
};

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error_tok(Token *tok, char *fmt, ...);
Token *peek(char *s);
Token *consume(char *op);
char *strndupl(char *p, int len);
Token *consume_ident();
void expect(char *op);
int expect_number();
char *expect_ident();
bool at_eof();
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
Token *tokenize();

// グローバル変数

extern char *user_input;
extern Token *token;

/*
******** PARSER ********
*/

// 変数
typedef struct Var Var;
struct Var {
  char *name;    // 変数名
  Type *ty;      // 型
  bool is_local; // local or global

  // local variable
  int offset; // Offset from RBP
};

typedef struct VarList VarList;
struct VarList {
  VarList *next;
  Var *var;
};

// AST node
typedef enum {
  NODE_ADD,       // +
  NODE_SUB,       // -
  NODE_MUL,       // *
  NODE_DIV,       // /
  NODE_EQ,        // ==
  NODE_NE,        // !=
  NODE_LT,        // <
  NODE_LE,        // <=
  NODE_ASSIGN,    // =
  NODE_ADDR,      // unary &
  NODE_DEREF,     // unary *
  NODE_RETURN,    // "return"
  NODE_IF,        // "if"
  NODE_WHILE,     // "while"
  NODE_FOR,       // "for"
  NODE_SIZEOF,    // "sizeof"
  NODE_BLOCK,     // { ... }
  NODE_FUNCALL,   // Function call
  NODE_EXPR_STMT, // Expression statement
  NODE_VAR,       // 変数
  NODE_NUM,       // 整数
  NODE_NULL,      // Empty statement
} NodeKind;

// AST node type
typedef struct Node Node;
struct Node {
  NodeKind kind; // Node kind
  Node *next;    // Next node
  Type *ty;      // e.g. int, pointer to int
  Token *tok;    // Representative token

  Node *lhs; // Left-hand side
  Node *rhs; // Right-hand side

  // "if" | "while" | "for" statement
  Node *cond;
  Node *then;
  Node *els;
  Node *init;
  Node *inc;

  // Block
  Node *body;

  // Function call
  char *funcname;
  Node *args;

  Var *var; // Used if kind == NODE_VAR
  int val;  // Used if kind == NODE_NUM
};

typedef struct Function Function;
struct Function {
  Function *next;
  char *name;
  VarList *params;
  Node *node;
  VarList *locals;
  int stack_size;
};

typedef struct {
  VarList *globals;
  Function *fns;
} Program;

Program *program();

/*
******** CODE GENERATOR ********
*/

void codegen(Program *prog);

/*
******** TYPE ********
*/

typedef enum { TY_INT, TY_PTR, TY_ARRAY } TypeKind;

struct Type {
  TypeKind kind;
  Type *base;
  int array_size;
};

Type *int_type();
Type *pointer_to(Type *base);
Type *array_of(Type *base, int size);
int size_of(Type *ty);

void add_type(Program *prog);

#endif // POACC_H
