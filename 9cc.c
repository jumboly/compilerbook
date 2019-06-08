#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 可変長ベクタ
typedef struct {
    void **data;
    int capacity;
    int len;
} Vector;

Vector *new_vector() {
    Vector *vec = malloc(sizeof(Vector));
    vec->data = malloc(sizeof(void *) * 16);
    vec->capacity = 16;
    vec->len = 0;
    return vec;
}

void vec_push(Vector *vec, void *elem) {
    if (vec->capacity == vec->len) {
        vec->capacity *= 2;
        vec->data = realloc(vec->data, sizeof(void *) * vec->capacity);
    }
    vec->data[vec->len++] = elem;
}

// トークンの型を表す値
enum {
    TK_NUM = 256,   // 整数
    TK_EQ,          // ==
    TK_NE,          // !=
    TK_LE,          // <=
    TK_GE,          // >=
    TK_EOF,         // 入力終わり
};

// トークンの型
typedef struct {
    int ty;         // トークンの型
    int val;        // tyがTK_NUMの場合、その整数
    char *input;    // トークン文字列（エラーメッセージ用）
} Token;

// 入力プログラム
char *user_input;

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

// user_inputが指している文字列を
// トークンに分割してtokensに保存する
void tokenize() {
    tokens = new_vector();
    char *p = user_input;

    int i = 0;
    while (*p) {
        // 空白文字をスキップ
        if (isspace(*p)) {
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

        if (strchr("+-*/()<>", *p) != NULL) {
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

// ノードの型を表す値
enum {
    ND_NUM = 256,   // 整数のノードの型
};

// ノードの型
typedef struct Node {
    int ty;             // 演算子かND_NUM
    struct Node *lhs;   // 左辺
    struct Node *rhs;   // 右辺
    int val;            // tyがND_NUMの場合のみ使う
} Node;

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

    error_at(get_token(pos)->input, "数値でも開きカッコでもないトークンです");
}

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

Node *expr() {
    return equality();
}

void gen(Node *node) {
    if (node->ty == ND_NUM) {
        printf("  push %d\n", node->val);
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch (node->ty) {
    case '+':
        printf("  add rax, rdi\n");
        break;
    case '-':
        printf("  sub rax, rdi\n");
        break;
    case '*':
        printf("  imul rdi\n");
        break;
    case '/':
        printf("  cqo\n");
        printf("  idiv rdi\n");
        break;
    case TK_EQ: // ==
        printf("  cmp rax, rdi\n");
        printf("  sete al\n");
        printf("  movzb rax, al\n");
        break;
    case TK_NE: // !=
        printf("  cmp rax, rdi\n");
        printf("  setne al\n");
        printf("  movzb rax, al\n");
        break;
    case '<':
        printf("  cmp rax, rdi\n");
        printf("  setl al\n");
        printf("  movzb rax, al\n");
        break;
    case TK_LE:
        printf("  cmp rax, rdi\n");
        printf("  setle al\n");
        printf("  movzb rax, al\n");
        break;
    }

    printf("  push rax\n");
}

void expect(int line, int expected, int actual) {
    if (expected == actual) {
        return;
    }
    fprintf(stderr, "%d: %d expected, but got %d\n",
        line, expected, actual);
    exit(1);
}

void runtest() {
    Vector *vec = new_vector();
    expect(__LINE__, 0, vec->len);

    for (int i = 0; i < 100; i++) {
        vec_push(vec, (void *)i);
    }

    expect(__LINE__, 100, vec->len);
    expect(__LINE__, 0, (long)vec->data[0]);
    expect(__LINE__, 50, (long)vec->data[50]);
    expect(__LINE__, 99, (long)vec->data[99]);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    // テスト
    if (strcmp(argv[1], "-test") == 0) {
        runtest();
        exit(0);
    }

    // トークナイズする
    user_input = argv[1];
    tokenize();

    // 構文木を組み立てる
    Node *node = expr();

    // アセンブリの前半分を出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // 抽象構文木を下りながらコード生成
    gen(node);

    // スタックトップに式全体の値が残っているはずなので
    // それをRAXにロードして関数からの戻り値とする
    printf("  pop rax\n");
    printf("  ret\n");
    return 0;
}
