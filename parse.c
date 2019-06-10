#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "9cc.h"

// 入力プログラム
char *user_input;

// エラーを報告するための関数
// printfと同じ引数を取る
void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// エラー箇所を報告するための関数
void error_at(char *loc, char *msg) {
    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, ""); // pos個の空白を出力
    fprintf(stderr, "^ %s\n", msg);
    exit(1);
}

// トークナイズした結果のトークン列を保存する
Vector *tokens;

Token *get_token(int i) {
    return (Token *)tokens->data[i];
}

void push_token(int ty, char *input) {
    Token *token = malloc(sizeof(Token));
    token->ty = ty;
    token->input = input;
    vec_push(tokens, token);
}

void push_token_num(int val, char *input) {
    Token *token = malloc(sizeof(Token));
    token->ty = TK_NUM;
    token->val = val;
    token->input = input;
    vec_push(tokens, token);
}

// user_inputが指している文字列を
// トークンに分割してtokensに保存する
void tokenize(char *input) {
    user_input = input;
    tokens = new_vector();

    char *p = input;
    while (*p) {
        // 空白文字をスキップ
        if (isspace(*p)) {
            p++;
            continue;
        }

        if ('a' <= *p && *p <= 'z') {
            push_token(TK_IDENT, p);
            p++;
            continue;
        }

        if (strncmp(p, "==", 2) == 0) {
            push_token(TK_EQ, p);
            p+=2;
            continue;
        }
        if (strncmp(p, "!=", 2) == 0) {
            push_token(TK_NE, p);
            p+=2;
            continue;
        }
        if (strncmp(p, "<=", 2) == 0) {
            push_token(TK_LE, p);
            p+=2;
            continue;
        }
        if (strncmp(p, ">=", 2) == 0) {
            push_token(TK_GE, p);
            p+=2;
            continue;
        }

        if (strchr("+-*/()<>=;", *p) != NULL) {
            push_token(*p, p);
            p++;
            continue;
        }

        if (isdigit(*p)) {
            char *input = p;
            int val = strtol(p, &p, 10);
            push_token_num(val, input);
            continue;
        }

        error_at(p, "トークナイズできません");
    }

    push_token(TK_EOF, p);
}

Node *code[100];

// ノードの作成
Node *new_node(int ty, Node *lhs, Node *rhs) {
    Node *node = malloc(sizeof(Node));
    node->ty = ty;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

// 整数ノードの作成
Node *new_node_num(int val) {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_NUM;
    node->val = val;
    return node;
}

int pos = 0;
// ノードが指定した型なら次のトークンに移動
int consume(int ty) {
    if (get_token(pos)->ty != ty) {
        return 0;
    }
    pos++;
    return 1;
}

Node *expr();
// term := NUM | "(" expr ")"

Node *term() {
    if (consume('(')) {
        Node *node = expr();
        if (!consume(')')) {
            error_at(get_token(pos)->input, "開きカッコに対応する閉じカッコがありません");
        }
        return node;
    }

    if (get_token(pos)->ty == TK_NUM) {
        return new_node_num(get_token(pos++)->val);
    }

    if (get_token(pos)->ty == TK_IDENT) {
        char varname = get_token(pos++)->input[0];

        Node *node = malloc(sizeof(Node));
        node->ty = ND_LVAR;
        node->offset = (varname - 'a' + 1) * 8;
        return node;
    }

    error_at(get_token(pos)->input, "数値でも開きカッコでもないトークンです");
}

// unary := '+' term

Node *unary() {
    if (consume('+')) {
        return term();
    }
    if (consume('-')) {
        return new_node('-', new_node_num(0), term());
    }
    return term();
}

Node *mul() {
    Node *node = unary();

    for (;;) {
        if (consume('*')) {
            node = new_node('*', node, unary());
        } else if (consume('/')) {
            node = new_node('/', node, unary());
        } else {
            return node;
        }
    }
}

Node *add() {
    Node *node = mul();

    for (;;) {
        if (consume('+')) {
            node = new_node('+', node, mul());
        } else if (consume('-')) {
            node = new_node('-', node, mul());
        } else {
            return node;
        }
    }
}

Node *relational() {
    Node *node = add();

    for (;;) {
        if (consume('<')) {
            node = new_node('<', node, add());
        } else if (consume(TK_LE)) {
            node = new_node(TK_LE, node, add());
        } else if (consume('>')) {
            // < に読み替える
            node = new_node('<', add(), node);
        } else if (consume(TK_GE)) {
            // <= に読み替える
            node = new_node(TK_LE, add(), node);
        } else {
            return node;
        }
    }
}

Node *equality() {
    Node *node = relational();

    for (;;) {
        if (consume(TK_EQ)) {
            node = new_node(TK_EQ, node, relational());
        } else if (consume(TK_NE)) {
            node = new_node(TK_NE, node, relational());
        } else {
            return node;
        }
    }
}

Node *assign() {
    Node *node = equality();
    if (consume('=')) {
        node = new_node('=', node, assign());
    }
    return node;
}

Node *expr() {
    return assign();
}

Node *stmt() {
    Node *node = expr();
    if (!consume(';')) {
        error_at(get_token(pos)->input, "';'ではないトークンです");
    }
    return node;
}

void program() {
    int i = 0;
    while (get_token(pos)->ty != TK_EOF) {
        code[i++] = stmt();
    }
    code[i] = NULL;
}
