import json

def reconstruct_c_code(ast_json_path):
    with open(ast_json_path, 'r') as f:
        ast = json.load(f)

    funcs = []

    def get_type(node):
        if not node:
            return "void"
        names = node.get("names")
        if isinstance(names, list) and len(names) > 0:
            return names[0]
        return "void"

    def parse_params(param_list):
        params = []
        if not isinstance(param_list, list):
            return params
        for param in param_list:
            try:
                param_type_node = param["type"]["type"]
                param_type = get_type(param_type_node)
                param_name = param["type"].get("declname", "arg")
                params.append((param_type, param_name))
            except:
                continue
        return params

    for ext in ast.get("ext", []):
        if ext["_nodetype"] == "FuncDef":
            func = {}
            decl = ext["decl"]
            func["name"] = decl["name"]
            type_node = decl["type"]["type"]
            ret_type_node = type_node.get("type")
            func["ret_type"] = get_type(ret_type_node)

            args_node = type_node.get("args", {}).get("params", [])
            func["params"] = parse_params(args_node)

            # Count if-statements
            def count_ifs(node):
                count = 0
                if isinstance(node, dict):
                    if node.get("_nodetype") == "If":
                        count += 1
                    for v in node.values():
                        count += count_ifs(v)
                elif isinstance(node, list):
                    for item in node:
                        count += count_ifs(item)
                return count

            func["ifs"] = count_ifs(ext.get("body", {}))
            funcs.append(func)

    # Generate C code
    output = []
    for func in funcs:
        params_str = ', '.join([f"{t} {n}" for t, n in func["params"]]) if func["params"] else "void"
        output.append(f"{func['ret_type']} {func['name']}({params_str}) {{")
        for i in range(func["ifs"]):
            output.append("    if (1) {\n        // 추정된 if 문\n    }")
        output.append("    // 함수 본문 생략\n}\n")

    return '\n'.join(output)

# 사용 예시
if __name__ == "__main__":
    source_code = reconstruct_c_code("ast.json")
    print("=== 추정된 C 소스코드 ===\n")
    print(source_code)
