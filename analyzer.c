아래는 완성된 코드입니다. JSON 데이터를 파일에서 읽어와 파싱하고, 특정 정보를 추출하는 기능을 포함합니다. `json-c` 라이브러리를 활용하여 작성되었습니다.

---

### 전체 코드

```c
#include 
#include 
#include 
#include 
#include 

// json_key_is_null 함수는 json_object에서 특정 키가 null인지 확인합니다.
bool json_key_is_null(json_object* json, const char* key) {
    if (json == NULL || key == NULL || *key == '\0')
        return false;

    json_object* value;
    if (!json_object_object_get_ex(json, key, &value)) {
        return true; // 키가 존재하지 않거나 값이 NULL일 경우 true 반환
    }
    return false;
}

// get_param 함수는 함수의 파라미터를 추출합니다.
void get_param(json_object* decl) {
    json_object* type = json_object_object_get(decl, "type");
    if (type == NULL) {
        printf("\tNOP\n");
        return;
    }

    json_object* args = json_object_object_get(type, "args");
    if (args == NULL || json_object_get_type(args) == json_type_null) {
        printf("\tNOP\n");
        return;
    }

    json_object* params = json_object_object_get(args, "params");
    if (params == NULL || json_object_get_type(params) != json_type_array) {
        printf("\tNOP\n");
        return;
    }

    for (int i = 0; i < json_object_array_length(params); i++) {
        json_object* param = json_object_array_get_idx(params, i);
        json_object* param_type = json_object_object_get(param, "type");

        if (param_type != NULL) {
            json_object* names = json_object_object_get(param_type, "names");
            if (names != NULL && json_object_get_type(names) == json_type_array) {
                printf("\t%s ", json_object_get_string(json_object_array_get_idx(names, 0)));
            }
        }

        json_object* declname = json_object_object_get(param_type, "declname");
        if (declname != NULL && json_object_get_type(declname) != json_type_null) {
            printf("%s\n", json_object_get_string(declname));
        } else {
            printf("\n");
        }
    }
}

// get_if 함수는 함수 내의 if 조건 개수를 추출합니다.
int get_if(json_object* block_items) {
    int if_cnt = 0;

    if (block_items == NULL || json_object_get_type(block_items) != json_type_array)
        return if_cnt;

    for (int i = 0; i < json_object_array_length(block_items); i++) {
        json_object* item = json_object_array_get_idx(block_items, i);
        const char* nodetype = json_object_get_string(json_object_object_get(item, "_nodetype"));

        if (nodetype != NULL && strcmp(nodetype, "If") == 0) {
            if_cnt++;

            // IfTrue 처리
            json_object* iftrue = json_object_object_get(item, "iftrue");
            if (iftrue != NULL && !json_key_is_null(iftrue, "block_items")) {
                if_cnt += get_if(json_object_object_get(iftrue, "block_items"));
            }

            // IfFalse 처리
            json_object* iffalse = json_object_object_get(item, "iffalse");
            if (iffalse != NULL && !json_key_is_null(iffalse, "block_items")) {
                if_cnt += get_if(json_object_object_get(iffalse, "block_items"));
            }
        } else if (nodetype != NULL && strcmp(nodetype, "While") == 0) {
            // While 처리
            json_object* stmt = json_object_object_get(item, "stmt");
            if (stmt != NULL && !json_key_is_null(stmt, "block_items")) {
                if_cnt += get_if(json_object_object_get(stmt, "block_items"));
            }
        } else if (nodetype != NULL && strcmp(nodetype, "For") == 0) {
            // For 처리
            json_object* stmt = json_object_object_get(item, "stmt");
            if (stmt != NULL && !json_key_is_null(stmt, "block_items")) {
                if_cnt += get_if(json_object_object_get(stmt, "block_items"));
            }
        }
    }

    return if_cnt;
}

int main() {
    FILE* file = fopen("./ast.json", "rb");
    if (file == NULL) {
        perror("파일 열기 실패");
        return 1;
    }

    int file_size;
    char* json_data;

    // 파일 크기 확인 및 메모리 할당
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    rewind(file);

    json_data = (char*)malloc(sizeof(char) * file_size + 1); // +1은 널 문자를 위해 추가
    if (json_data == NULL) {
        perror("메모리 할당 실패");
        fclose(file);
        return 1;
    }

    fread(json_data, 1, file_size, file);
    json_data[file_size] = '\0'; // 널 문자 추가
    fclose(file);

    // JSON 데이터 파싱
    struct json_tokener* tokener = json_tokener_new();
    struct json_object* root_json = json_tokener_parse_ex(tokener, json_data, file_size);
    free(json_data);

    if (root_json == NULL) {
        fprintf(stderr, "JSON 파싱 실패: %s\n", json_tokener_error_desc(json_tokener_get_error(tokener)));
        json_tokener_free(tokener);
        return 1;
    }
    json_tokener_free(tokener);

    struct json_object* ext;
    int func_cnt = 0;

    // ext 객체 가져오기
    if (json_object_object_get_ex(root_json, "ext", &ext)) {
        for (int i = 0; i < json_object_array_length(ext); i++) {
            struct json_object* datas = json_object_array_get_idx(ext, i);

            const char* nodetype = json_object_get_string(json_object_object_get(datas, "_nodetype"));
            if (nodetype != NULL && strcmp(nodetype, "FuncDef") == 0) {
                func_cnt++;

                struct json_object* decl = json_object_object_get(datas, "decl");
                struct json_object* body = json_object_object_get(datas, "body");
                struct json_object* block_items = json_object_object_get(body, "block_items");

                // 함수 이름 추출
                printf("Function Name: %s\n", 
                    json_object_get_string(json_object_object_get(decl, "name")));

                // 리턴 타입 추출
                struct json_object* type = json_object_object_get(decl, "type");
                type = json_object_object_get(type, "type");
                type = json_object_object_get(type, "type");
                struct json_object* return_type = json_object_object_get(type, "names");
                printf("Return Type: %s\n", 
                    json_object_get_string(json_object_array_get_idx(return_type, 0)));

                // 파라미터 추출
                printf("Parameters: \n");
                get_param(decl);

                // if 조건 개수 추출
                printf("\nif count: %d\n", get_if(block_items));
                printf("\n--------------------------------------------\n\n");
            }
        }
    }

    printf("Total Functions: %d\n", func_cnt);

    // JSON 객체 메모리 해제
    json_object_put(root_json);

    return 0;
}

