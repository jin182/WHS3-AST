#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

typedef struct {
    char* name;
    char* type;
} Parameter;

typedef struct {
    char* name;
    char* return_type;
    Parameter** parameters;
    int parameter_count;
    int if_condition_count;
} FunctionInfo;

// 함수 정보 구조체에 대한 메모리 해제 함수
void free_function_info(FunctionInfo* info) {
    if (info == NULL) return;
    
    free(info->name);
    free(info->return_type);
    
    for (int i = 0; i < info->parameter_count; i++) {
        free(info->parameters[i]->name);
        free(info->parameters[i]->type);
        free(info->parameters[i]);
    }
    free(info->parameters);
}

// 함수 개수 카운트
int count_functions(cJSON* node) {
    if (!node) return 0;
    
    int count = 0;
    
    if (cJSON_IsObject(node)) {
        cJSON* nodetype = cJSON_GetObjectItemCaseSensitive(node, "_nodetype");
        if (nodetype && cJSON_IsString(nodetype)) {
            if (strcmp(nodetype->valuestring, "FuncDef") == 0) {
                count++;
            }
        }
        
        // 재귀 탐색
        cJSON* child = node->child;
        while (child) {
            count += count_functions(child);
            child = child->next;
        }
    }
    
    // 배열 처리
    if (cJSON_IsArray(node)) {
        int size = cJSON_GetArraySize(node);
        for (int i = 0; i < size; i++) {
            cJSON* item = cJSON_GetArrayItem(node, i);
            count += count_functions(item);
        }
    }
    
    return count;
}

// 함수 리턴 타입 추출
char* extract_function_return_type(cJSON* func_node) {
    if (!func_node) return strdup("Unknown");
    
    cJSON* decl = cJSON_GetObjectItemCaseSensitive(func_node, "decl");
    if (!decl) return strdup("Unknown");
    
    cJSON* type = cJSON_GetObjectItemCaseSensitive(decl, "type");
    if (!type) return strdup("Unknown");
    
    cJSON* type_type = cJSON_GetObjectItemCaseSensitive(type, "type");
    if (!type_type) return strdup("Unknown");
    
    cJSON* names = cJSON_GetObjectItemCaseSensitive(type_type, "names");
    if (!names || !cJSON_IsArray(names)) return strdup("Unknown");
    
    cJSON* name = cJSON_GetArrayItem(names, 0);
    return strdup(name->valuestring);
}

// 함수 이름 추출
char* extract_function_name(cJSON* func_node) {
    if (!func_node) return strdup("Unknown");
    
    cJSON* decl = cJSON_GetObjectItemCaseSensitive(func_node, "decl");
    if (!decl) return strdup("Unknown");
    
    cJSON* name = cJSON_GetObjectItemCaseSensitive(decl, "name");
    if (!name) return strdup("Unknown");
    
    return strdup(name->valuestring);
}

// 함수 파라미터 추출
Parameter** extract_function_parameters(cJSON* func_node, int* param_count) {
    if (!func_node || !param_count) return NULL;
    
    *param_count = 0;
    Parameter** parameters = NULL;
    
    cJSON* decl = cJSON_GetObjectItemCaseSensitive(func_node, "decl");
    if (!decl) return NULL;
    
    cJSON* type = cJSON_GetObjectItemCaseSensitive(decl, "type");
    if (!type) return NULL;
    
    cJSON* args = cJSON_GetObjectItemCaseSensitive(type, "args");
    if (!args) return NULL;
    
    cJSON* params = cJSON_GetObjectItemCaseSensitive(args, "params");
    if (!params || !cJSON_IsArray(params)) return NULL;
    
    int size = cJSON_GetArraySize(params);
    parameters = malloc(size * sizeof(Parameter*));
    
    for (int i = 0; i < size; i++) {
        cJSON* param = cJSON_GetArrayItem(params, i);
        parameters[i] = malloc(sizeof(Parameter));
        
        // 파라미터 타입 추출
        cJSON* param_type = cJSON_GetObjectItemCaseSensitive(param, "type");
        cJSON* type_details = cJSON_GetObjectItemCaseSensitive(param_type, "type");
        cJSON* type_names = cJSON_GetObjectItemCaseSensitive(type_details, "names");
        
        if (type_names && cJSON_IsArray(type_names)) {
            cJSON* type_name = cJSON_GetArrayItem(type_names, 0);
            parameters[i]->type = strdup(type_name->valuestring);
        } else {
            parameters[i]->type = strdup("Unknown");
        }
        
        // 파라미터 이름 추출
        cJSON* name = cJSON_GetObjectItemCaseSensitive(param, "name");
        if (name) {
            parameters[i]->name = strdup(name->valuestring);
        } else {
            parameters[i]->name = strdup("Unknown");
        }
    }
    
    *param_count = size;
    return parameters;
}

// if 조건문 개수 카운트
int count_if_conditions(cJSON* node) {
    if (!node) return 0;
    
    int count = 0;
    
    if (cJSON_IsObject(node)) {
        cJSON* nodetype = cJSON_GetObjectItemCaseSensitive(node, "_nodetype");
        if (nodetype && cJSON_IsString(nodetype)) {
            if (strcmp(nodetype->valuestring, "If") == 0) {
                count++;
            }
        }
        
        // 재귀 탐색
        cJSON* child = node->child;
        while (child) {
            count += count_if_conditions(child);
            child = child->next;
        }
    }
    
    // 배열 처리
    if (cJSON_IsArray(node)) {
        int size = cJSON_GetArraySize(node);
        for (int i = 0; i < size; i++) {
            cJSON* item = cJSON_GetArrayItem(node, i);
            count += count_if_conditions(item);
        }
    }
    
    return count;
}

int main() {
    // JSON 파일 읽기
    FILE* file = fopen("ast.json", "r");
    if (!file) {
        printf("파일을 열 수 없습니다.\n");
        return 1;
    }
    
    // 파일 크기 계산
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
    
    // 메모리 할당 및 파일 읽기
    char* json_string = malloc(file_size + 1);
    fread(json_string, 1, file_size, file);
    json_string[file_size] = '\0';
    fclose(file);
    
    // JSON 파싱
    cJSON* root = cJSON_Parse(json_string);
    free(json_string);
    
    if (!root) {
        printf("JSON 파싱 실패\n");
        return 1;
    }
    
    // 함수 개수 카운트
    int total_functions = count_functions(root);
    printf("총 함수 개수: %d\n", total_functions);
    
    // 함수 정보 배열 동적 할당
    FunctionInfo* functions = malloc(total_functions * sizeof(FunctionInfo));
    
    // 함수 정보 추출을 위한 인덱스
    int func_index = 0;
    
    // 재귀 함수로 함수 정보 추출
    void extract_function_info(cJSON* node) {
        if (!node) return;
        
        if (cJSON_IsObject(node)) {
            cJSON* nodetype = cJSON_GetObjectItemCaseSensitive(node, "_nodetype");
            if (nodetype && cJSON_IsString(nodetype)) {
                if (strcmp(nodetype->valuestring, "FuncDef") == 0) {
                    // 현재 함수의 리턴 타입 추출
                    functions[func_index].return_type = extract_function_return_type(node);
                    
                    // 함수 이름 추출
                    functions[func_index].name = extract_function_name(node);
                    
                    // 파라미터 추출
                    functions[func_index].parameters = extract_function_parameters(node, &functions[func_index].parameter_count);
                    
                    // if 조건문 개수 카운트
                    functions[func_index].if_condition_count = count_if_conditions(node);
                    
                    func_index++;
                }
            }
            
            // 재귀 탐색
            cJSON* child = node->child;
            while (child) {
                extract_function_info(child);
                child = child->next;
            }
        }
        
        // 배열 처리
        if (cJSON_IsArray(node)) {
            int size = cJSON_GetArraySize(node);
            for (int i = 0; i < size; i++) {
                cJSON* item = cJSON_GetArrayItem(node, i);
                extract_function_info(item);
            }
        }
    }
    
    // 함수 정보 추출
    extract_function_info(root);
    
    // 추출된 함수 정보 출력
    printf("\n함수 상세 정보:\n");
    for (int i = 0; i < total_functions; i++) {
        printf("\n함수 %d:\n", i+1);
        printf("- 이름: %s\n", functions[i].name);
        printf("- 리턴 타입: %s\n", functions[i].return_type);
        
        printf("- 파라미터:\n");
        for (int j = 0; j < functions[i].parameter_count; j++) {
            printf("  * 이름: %s, 타입: %s\n", 
                   functions[i].parameters[j]->name, 
                   functions[i].parameters[j]->type);
        }
        
        printf("- if 조건문 개수: %d\n", functions[i].if_condition_count);
    }
    
    // 메모리 해제
    for (int i = 0; i < total_functions; i++) {
        free_function_info(&functions[i]);
    }
    free(functions);
    
    cJSON_Delete(root);
    
    return 0;
}
