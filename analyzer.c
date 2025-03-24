#include "stdio.h"
#include "json_c.h"

// json_key_is_null 함수는 json_object에서 특정 키가 null인지 확인합니다.
bool json_key_is_null(json_object* json, const char* key) {
    if (json == NULL || key == NULL || *key == '\0')
        return false;

    for (int i = 0; i <= json->last_index; i++) {
        if (strcmp(json->keys[i], key) == 0) {
            return true;
        }
    }

    return false;
}

// get_param 함수는 함수의 파라미터를 추출합니다.
void get_param(json_value decl) {
    json_value type = json_get(decl, "type");
    json_value args = json_get(type, "args");

    if (args.type == JSON_NULL) {
        printf("\tNOP\n");
        return;
    }

    json_value param = json_get(args, "params");
    json_value args_name;

    for (int i = 0; i < json_len(param); i++) {
        args_name = json_get(json_get(param, i), "type");
        printf("\t%s ", json_get_string(json_get(json_get(args_name, "type"),"names"),0));

        if (json_get(args_name, "declname").type != JSON_NULL) {
            printf("%s\n", json_get_string(args_name, "declname"));
        } else {
            printf("\n");
        }
    }
}

// get_if 함수는 함수 내의 if 조건 개수를 추출합니다.
int get_if(json_value block_item) {
    json_value item;
    int if_cnt = 0;

    for (int i = 0; i < json_len(block_item); i++) {
        item = json_get(block_item, i);

        if (!strcmp(json_get_string(item, "_nodetype"), "If")) {
            if_cnt++;

            if (json_get(item, "iftrue").type != JSON_NULL) {
                if (json_key_is_null(json_get(item, "iftrue").value, "block_items"))
                    if_cnt += get_if(json_get(json_get(item, "iftrue"), "block_items"));
            }

            else if (json_get(item, "iffalse").type != JSON_NULL) {
                if (json_key_is_null(json_get(item, "iffalse").value, "block_items"))
                    if_cnt += get_if(json_get(json_get(item, "iffalse"), "block_items"));
            }
        }

        else if (!strcmp(json_get_string(item, "_nodetype"), "While")) {
            if (json_key_is_null(json_get(item, "stmt").value, "block_items"))
                if_cnt+=get_if(json_get(json_get(item, "stmt"),"block_items"));
        }

        else if (!strcmp(json_get_string(item, "_nodetype"), "For")) {
            if (json_key_is_null(json_get(item, "stmt").value, "block_items"))
                if_cnt += get_if(json_get(json_get(item, "stmt"), "block_items"));
        }
    }

    return if_cnt;
}

int main() {
    FILE* file = fopen("./ast.json", "rb");
    int file_size;
    char* json_data;

    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    rewind(file);

    json_data = (char*)malloc(sizeof(char) * file_size);
    fread(json_data, 1, file_size, file);
    strcat(json_data, "\0");
    fclose(file);

    json_value json = json_create(json_data);
    json_value ext = json_get(json, "ext");

    int func_cnt = 0;
    for (int i = 0; i < json_len(ext); i++) {
        json_value datas = json_get(ext, i);

        if (!strcmp(json_get_string(datas, "_nodetype"), "FuncDef")) {
            func_cnt++;
            json_value decl = json_get(datas, "decl");
            json_value body = json_get(datas, "body");
            json_value block_item = json_get(body, "block_items");

            // 함수 이름 추출
            printf("Function Name: %s\n", json_get_string(decl, "name"));

            // 리턴 타입 추출
            json_value type = json_get(decl, "type");
            type = json_get(type, "type");
            type = json_get(type, "type");
            json_value return_type = json_get(type, "names");
            printf("Return Type: %s\n", json_get_string(return_type, 0));

            // 파라미터 추출
            printf("Parameters: \n");
            get_param(decl);

            // if 조건 개수 추출
            printf("\nif count: %d\n", get_if(block_item));
            printf("\n--------------------------------------------\n\n");
        }
    }

    printf("Total Functions: %d\n", func_cnt);
    free(json_data);
    return 0;
}
