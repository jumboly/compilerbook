#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "9cc.h"

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
    tokenize(argv[1]);

    // 構文木を組み立てる
    program();

    // アセンブリの前半分を出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // プロローグ
    // 変数26個分の領域を確保する
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, 208\n"); // 26 * 8

    // 戦闘の式から順にコード生成
    for (int i = 0; code[i]; i++) {
        gen(code[i]);

        // 式の評価結果としてスタックに一つの値が残っているはずなので
        // スタックが溢れないようにポップしておく
        printf("  pop rax\n");
    }

    // エピローグ
    // 最後の式の結果がRAXに残っているのでそれが返り値になる
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
    return 0;
}
