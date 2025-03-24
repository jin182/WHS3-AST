#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>


// Forward declarations
typedef struct ASTNode ASTNode;
ASTNode* from_json(const char* ast_json);
ASTNode* from_dict(json_t* node_dict);
void* parse_coord(const char* coord_str);
void* convert_to_obj(json_t* value);
char* generate_code(ASTNode* ast_node);

// Represents a coordinate in the source code
typedef struct {
    char* filename;
    int line;
    int column;
} Coord;

// Generic AST node structure
struct ASTNode {
    char* node_type;
    Coord* coord;
    // Additional fields would be added dynamically based on node type
    void** fields;       // Array of pointers to field values
    char** field_names;  // Array of field names
    int field_count;     // Number of fields
};

// Create a coordinate from a string representation
Coord* parse_coord(const char* coord_str) {
    if (coord_str == NULL) {
        return NULL;
    }
    
    Coord* coord = (Coord*)malloc(sizeof(Coord));
    if (!coord) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    
    char* str_copy = strdup(coord_str);
    char* token = strtok(str_copy, ":");
    
    coord->filename = token ? strdup(token) : NULL;
    
    token = strtok(NULL, ":");
    coord->line = token ? atoi(token) : 0;
    
    token = strtok(NULL, ":");
    coord->column = token ? atoi(token) : 0;
    
    free(str_copy);
    return coord;
}

// Convert JSON value to appropriate C object
void* convert_to_obj(json_t* value) {
    if (!value) return NULL;
    
    if (json_is_object(value)) {
        return from_dict(value);
    } 
    else if (json_is_array(value)) {
        size_t index;
        json_t* element;
        size_t array_size = json_array_size(value);
        
        // Create an array of void pointers
        void** array = (void**)malloc((array_size + 1) * sizeof(void*));
        if (!array) {
            fprintf(stderr, "Memory allocation failed\n");
            return NULL;
        }
        
        // Convert each array element
        for (index = 0; index < array_size; index++) {
            element = json_array_get(value, index);
            array[index] = convert_to_obj(element);
        }
        array[array_size] = NULL;  // Null terminate the array
        
        return array;
    } 
    else if (json_is_string(value)) {
        return strdup(json_string_value(value));
    } 
    else if (json_is_integer(value)) {
        int* num = (int*)malloc(sizeof(int));
        if (num) *num = (int)json_integer_value(value);
        return num;
    } 
    else if (json_is_real(value)) {
        double* num = (double*)malloc(sizeof(double));
        if (num) *num = json_real_value(value);
        return num;
    } 
    else if (json_is_boolean(value)) {
        int* boolean = (int*)malloc(sizeof(int));
        if (boolean) *boolean = json_is_true(value) ? 1 : 0;
        return boolean;
    } 
    else if (json_is_null(value)) {
        return NULL;
    }
    
    return NULL;
}

// Create an AST node from a JSON dictionary
ASTNode* from_dict(json_t* node_dict) {
    if (!json_is_object(node_dict)) {
        return NULL;
    }
    
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    
    // Get node type
    json_t* nodetype_json = json_object_get(node_dict, "_nodetype");
    if (json_is_string(nodetype_json)) {
        node->node_type = strdup(json_string_value(nodetype_json));
    } else {
        node->node_type = strdup("Unknown");
    }
    
    // Count fields (excluding _nodetype)
    node->field_count = json_object_size(node_dict) - 1;
    
    // Allocate arrays for field names and values
    node->field_names = (char**)malloc(node->field_count * sizeof(char*));
    node->fields = (void**)malloc(node->field_count * sizeof(void*));
    
    if (!node->field_names || !node->fields) {
        fprintf(stderr, "Memory allocation failed\n");
        free(node->node_type);
        free(node->field_names);
        free(node->fields);
        free(node);
        return NULL;
    }
    
    // Initialize field arrays
    memset(node->field_names, 0, node->field_count * sizeof(char*));
    memset(node->fields, 0, node->field_count * sizeof(void*));
    
    // Process each field
    const char* key;
    json_t* value;
    int field_index = 0;
    
    node->coord = NULL;  // Default
    
    json_object_foreach(node_dict, key, value) {
        if (strcmp(key, "_nodetype") == 0) {
            continue;  // Already processed
        }
        
        if (strcmp(key, "coord") == 0 && json_is_string(value)) {
            node->coord = parse_coord(json_string_value(value));
            continue;
        }
        
        // Add field name
        node->field_names[field_index] = strdup(key);
        
        // Convert and add field value
        node->fields[field_index] = convert_to_obj(value);
        
        field_index++;
    }
    
    return node;
}

// Parse JSON string into an AST node
ASTNode* from_json(const char* ast_json) {
    json_error_t error;
    json_t* root = json_loads(ast_json, 0, &error);
    
    if (!root) {
        fprintf(stderr, "JSON parsing error on line %d: %s\n", error.line, error.text);
        return NULL;
    }
    
    ASTNode* node = from_dict(root);
    json_decref(root);
    return node;
}

// Generate C code from AST node
// This is a placeholder - you'll need to implement the actual code generation logic
char* generate_code(ASTNode* ast_node) {
    if (!ast_node) {
        return strdup("// Empty AST\n");
    }
    
    // This would be a complex implementation that traverses the AST
    // and generates appropriate C code for each node type
    // For now, we just return a placeholder
    
    return strdup("// Code generation not yet implemented\n");
}

// Free memory used by an AST node
void free_ast_node(ASTNode* node) {
    if (!node) return;
    
    free(node->node_type);
    
    if (node->coord) {
        free(node->coord->filename);
        free(node->coord);
    }
    
    for (int i = 0; i < node->field_count; i++) {
        free(node->field_names[i]);
        // We would need a more complex function to free field values
        // based on their types (recursively free nested nodes, etc.)
        free(node->fields[i]);
    }
    
    free(node->field_names);
    free(node->fields);
    free(node);
}

int main() {
    FILE* file = fopen("ast.json", "r");
    if (!file) {
        fprintf(stderr, "Failed to open ast.json\n");
        return 1;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Read file content
    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        return 1;
    }
    
    fread(buffer, 1, size, file);
    buffer[size] = '\0';
    fclose(file);
    
    // Process AST
    ASTNode* ast_node = from_json(buffer);
    free(buffer);
    
    if (!ast_node) {
        fprintf(stderr, "Failed to parse AST\n");
        return 1;
    }
    
    // Generate code
    char* code = generate_code(ast_node);
    
    // Write to output file
    file = fopen("output.c", "w");
    if (!file) {
        fprintf(stderr, "Failed to open output.c for writing\n");
        free_ast_node(ast_node);
        free(code);
        return 1;
    }
    
    fputs(code, file);
    fclose(file);
    
    // Cleanup
    free_ast_node(ast_node);
    free(code);
    
    printf("Code generation completed successfully\n");
    return 0;
}