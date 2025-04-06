#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

// 매크로 정의
#define OBJ(o, key) cJSON_GetObjectItem(o, key)
#define ARR(o, idx) cJSON_GetArrayItem(o, idx)
#define IS_STR(n) (cJSON_IsString(n))
#define IS_ARR(n) (cJSON_IsArray(n))
#define ARR_SIZE(a) cJSON_GetArraySize(a)

typedef struct {
    char *type;
    char *name;
} Param;

typedef struct {
    char *name;
    char *retType;
    int ifs;
    int argc;
    Param args[10]; // 최대 10개
} Func;

#define MAX_FUNCS 100
Func funcs[MAX_FUNCS];
int funcCnt = 0;

const char *getType(cJSON *node) {
    if (!node) return "unknown";
    cJSON *names = OBJ(node, "names");
    if (IS_ARR(names) && ARR_SIZE(names) > 0) {
        cJSON *n = ARR(names, 0);
        if (IS_STR(n)) return n->valuestring;
    }
    return "unknown";
}

void parseFunc(cJSON *decl) {
    if (!decl) return;

    Func *f = &funcs[funcCnt];
    memset(f, 0, sizeof(Func));

    cJSON *name = OBJ(decl, "name");
    f->name = name ? strdup(name->valuestring) : strdup("unknown");

    cJSON *type = OBJ(decl, "type");
    cJSON *ret = OBJ(type, "type");
    cJSON *idType = ret ? OBJ(ret, "type") : NULL;
    f->retType = idType ? strdup(getType(idType)) : strdup("unknown");

    cJSON *params = OBJ(OBJ(type, "args"), "params");
    if (IS_ARR(params)) {
        int n = ARR_SIZE(params);
        for (int i = 0; i < n && i < 10; i++) {
            cJSON *p = ARR(params, i);
            cJSON *pt = OBJ(p, "type");
            cJSON *td = pt ? OBJ(pt, "type") : NULL;
            if (!td) continue;

            const char *t = getType(td);
            const char *n = "arg";
            cJSON *pn = OBJ(pt, "declname");
            if (pn && IS_STR(pn)) n = pn->valuestring;

            f->args[f->argc].type = strdup(t);
            f->args[f->argc].name = strdup(n);
            f->argc++;
        }
    }

    funcCnt++;
}

void countIf(cJSON *node, Func *f) {
    if (!node || !f) return;

    cJSON *type = OBJ(node, "_nodetype");
    if (type && !strcmp(type->valuestring, "If")) f->ifs++;

    cJSON *child;
    cJSON_ArrayForEach(child, node) {
        countIf(child, f);
    }
}

void traverse(cJSON *root) {
    cJSON *ext = OBJ(root, "ext");
    if (!IS_ARR(ext)) return;

    cJSON *node;
    cJSON_ArrayForEach(node, ext) {
        cJSON *nt = OBJ(node, "_nodetype");
        if (!nt || !IS_STR(nt)) continue;

        if (!strcmp(nt->valuestring, "Decl")) {
            cJSON *t = OBJ(node, "type");
            cJSON *inner = t ? OBJ(t, "_nodetype") : NULL;
            if (inner && !strcmp(inner->valuestring, "FuncDecl")) {
                parseFunc(node);
                funcs[funcCnt - 1].ifs = 0;
            }
        } else if (!strcmp(nt->valuestring, "FuncDef")) {
            cJSON *decl = OBJ(node, "decl");
            cJSON *body = OBJ(node, "body");
            parseFunc(decl);
            countIf(body, &funcs[funcCnt - 1]);
        }
    }
}

int main() {
    FILE *fp = fopen("ast.json", "r");
    if (!fp) {
        perror("파일 열기 실패");
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    rewind(fp);

    char *data = malloc(sz + 1);
    fread(data, 1, sz, fp);
    data[sz] = '\0';
    fclose(fp);

    cJSON *root = cJSON_Parse(data);
    free(data);
    if (!root) {
        fprintf(stderr, "JSON 파싱 실패\n");
        return 1;
    }

    traverse(root);
    cJSON_Delete(root);

    // 출력
    printf("==== 함수 분석 결과 ====\n");
    printf("총 %d개 함수\n", funcCnt);
    for (int i = 0; i < funcCnt; i++) {
        Func *f = &funcs[i];
        printf("\n[%d] %s\n", i + 1, f->name);
        printf("  - 반환 타입: %s\n", f->retType);
        printf("  - 파라미터 %d개:\n", f->argc);
        for (int j = 0; j < f->argc; j++) {
            printf("    - %s %s\n", f->args[j].type, f->args[j].name);
        }
        printf("  - if문 개수: %d\n", f->ifs);
    }

    return 0;
}

