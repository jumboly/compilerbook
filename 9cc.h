// 可変長ベクタ
typedef struct {
    void **data;
    int capacity;
    int len;
} Vector;

// 可変長ベクタの作成
Vector *new_vector();

// 可変長ベクタに要素を追加
void vec_push(Vector *vec, void *elem);

// テスト
void runtest();

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

// 入力文字列をトークンに分割してtokensに保存する
void tokenize(char *input);

Node *expr();

void gen(Node *node);
