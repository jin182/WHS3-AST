#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

typedef struct {
    char* name;
    char* type;
} Parameter;

typedef struct {
    int total_functions;
    char** return_types;
    Parameter** parameters;
    int total_if_conditions;
} ASTAnalysisResult;

// 함수 프로토타입 선언
int count_functions(cJSON* node);
char** extract_function_return_types(cJSON* node, int* count);
Parameter** extract_function_parameters(cJSON* node, int* count);
int count_if_conditions(cJSON* node);
void free_analysis_result(ASTAnalysisResult* result);

// 재귀적으로 함수 개수 카운트
int count_functions(cJSON* node) {
    if (!node) return 0;
    
    int count = 0;
    
    // 함수 타입 확인
    if (cJSON_IsObject(node)) {
        cJSON* type = cJSON_GetObjectItemCaseSensitive(node, "type");
        if (type && cJSON_IsString(type)) {
            if (strcmp(type->valuestring, "FunctionDeclaration") == 0 ||
                strcmp(type->valuestring, "MethodDefinition") == 0) {
                count++;
            }
        }
        
        // 모든 객체 속성 재귀 탐색
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
char** extract_function_return_types(cJSON* node, int* count) {
    if (!node || !count) return NULL;
    
    static char** return_types = NULL;
    static int total_count = 0;
    
    if (cJSON_IsObject(node)) {
        cJSON* type = cJSON_GetObjectItemCaseSensitive(node, "type");
        if (type && cJSON_IsString(type)) {
            if (strcmp(type->valuestring, "FunctionDeclaration") == 0 ||
                strcmp(type->valuestring, "MethodDefinition") == 0) {
                
                cJSON* return_type = cJSON_GetObjectItemCaseSensitive(node, "returnType");
                if (return_type && cJSON_IsObject(return_type)) {
                    cJSON* name = cJSON_GetObjectItemCaseSensitive(return_type, "name");
                    
                    return_types = realloc(return_types, (total_count + 1) * sizeof(char*));
                    return_types[total_count] = strdup(name ? name->valuestring : "Unknown");
                    total_count++;
                }
            }
        }
        
        // 재귀 탐색
        cJSON* child = node->child;
        while (child) {
            extract_function_return_types(child, count);
            child = child->next;
        }
    }
    
    // 배열 처리
    if (cJSON_IsArray(node)) {
        int size = cJSON_GetArraySize(node);
        for (int i = 0; i < size; i++) {
            cJSON* item = cJSON_GetArrayItem(node, i);
            extract_function_return_types(item, count);
        }
    }
    
    *count = total_count;
    return return_types;
}

// 함수 파라미터 추출
Parameter** extract_function_parameters(cJSON* node, int* count) {
    if (!node || !count) return NULL;
    
    static Parameter** parameters = NULL;
    static int total_count = 0;
    
    if (cJSON_IsObject(node)) {
        cJSON* type = cJSON_GetObjectItemCaseSensitive(node, "type");
        if (type && cJSON_IsString(type)) {
            if (strcmp(type->valuestring, "FunctionDeclaration") == 0 ||
                strcmp(type->valuestring, "MethodDefinition") == 0) {
                
                cJSON* params = cJSON_GetObjectItemCaseSensitive(node, "params");
                if (params && cJSON_IsArray(params)) {
                    int param_size = cJSON_GetArraySize(params);
                    
                    for (int i = 0; i < param_size; i++) {
                        cJSON* param = cJSON_GetArrayItem(params, i);
                        
                        parameters = realloc(parameters, (total_count + 1) * sizeof(Parameter*));
                        parameters[total_count] = malloc(sizeof(Parameter));
                        
                        cJSON* name = cJSON_GetObjectItemCaseSensitive(param, "name");
                        cJSON* param_type = cJSON_GetObjectItemCaseSensitive(param, "type");
                        
                        parameters[total_count]->name = strdup(name ? name->valuestring : "Unknown");
                        parameters[total_count]->type = strdup(param_type ? param_type->valuestring : "Unknown");
                        
                        total_count++;
                    }
                }
            }
        }
        
        // 재귀 탐색
        cJSON* child = node->child;
        while (child) {
            extract_function_parameters(child, count);
            child = child->next;
        }
    }
    
    // 배열 처리
    if (cJSON_IsArray(node)) {
        int size = cJSON_GetArraySize(node);
        for (int i = 0; i < size; i++) {
            cJSON* item = cJSON_GetArrayItem(node, i);
            extract_function_parameters(item, count);
        }
    }
    
    *count = total_count;
    return parameters;
}

// if 조건문 개수 카운트
int count_if_conditions(cJSON* node) {
    if (!node) return 0;
    
    int count = 0;
    
    if (cJSON_IsObject(node)) {
        cJSON* type = cJSON_GetObjectItemCaseSensitive(node, "type");
        if (type && cJSON_IsString(type)) {
            if (strcmp(type->valuestring, "IfStatement") == 0) {
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

// 분석 결과 메모리 해제
void free_analysis_result(ASTAnalysisResult* result) {
    if (!result) return;
    
    // 리턴 타입 해제
    for (int i = 0; i < result->total_functions; i++) {
        free(result->return_types[i]);
    }
    free(result->return_types);
    
    // 파라미터 해제
    for (int i = 0; i < result->total_functions; i++) {
        free(result->parameters[i]->name);
        free(result->parameters[i]->type);
        free(result->parameters[i]);
    }
    free(result->parameters);
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
    
    // 분석 결과 초기화
    ASTAnalysisResult result = {0};
    
    // 함수 개수 카운트
    result.total_functions = count_functions(root);
    
    // 리턴 타입 추출
    int return_type_count;
    result.return_types = extract_function_return_types(root, &return_type_count);
    
    // 파라미터 추출
    int parameter_count;
    result.parameters = extract_function_parameters(root, &parameter_count);
    
    // if 조건문 개수 카운트
    result.total_if_conditions = count_if_conditions(root);
    
    // 결과 출력
    printf("총 함수 개수: %d\n", result.total_functions);
    
    printf("\n리턴 타입:\n");
    for (int i = 0; i < return_type_count; i++) {
        printf("- %s\n", result.return_types[i]);
    }
    
    printf("\n파라미터:\n");
    for (int i = 0; i < parameter_count; i++) {
        printf("- 이름: %s, 타입: %s\n", 
               result.parameters[i]->name, 
               result.parameters[i]->type);
    }
    
    printf("\nif 조건문 개수: %d\n", result.total_if_conditions);
    
    // 메모리 해제
    free_analysis_result(&result);
    cJSON_Delete(root);
    
    return 0;
}
