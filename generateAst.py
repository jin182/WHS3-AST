import json

class CodeGen:
    def __init__(self):
        self.indent = 0

    def generate(self, node):
        if not node:
            return ""
        nodetype = node.get("_nodetype", "")

        # 최상위
        if nodetype == "FileAST":
            return self.visit_file(node)
        if nodetype == "FuncDef":
            return self.visit_funcdef(node)
        if nodetype == "Decl":
            return self.visit_decl(node)
        if nodetype == "FuncDecl":
            return self.visit_funcdecl(node)
        if nodetype == "Compound":
            return self.visit_compound(node)
        if nodetype == "Return":
            return self.visit_return(node)
        if nodetype == "If":
            return self.visit_if(node)
        if nodetype == "While":
            return self.visit_while(node)
        if nodetype == "BinaryOp":
            return self.visit_binop(node)
        if nodetype == "Assignment":
            return self.visit_assign(node)
        if nodetype == "FuncCall":
            return self.visit_funccall(node)
        if nodetype == "ID":
            return node.get("name", "")
        if nodetype == "Constant":
            return node.get("value", "")
        if nodetype == "ParamList":
            return self.visit_paramlist(node)
        if nodetype == "TypeDecl":
            return self.visit_typedecl(node)
        if nodetype == "PtrDecl":
            return self.visit_ptrdecl(node)
        if nodetype == "IdentifierType":
            return " ".join(node.get("names", []))
        if nodetype == "ExprList":
            exprs = node.get("exprs", [])
            return ", ".join(self.generate(e) for e in exprs)
        if nodetype == "ArrayRef":
            base = self.generate(node.get("name"))
            idx = self.generate(node.get("subscript"))
            return f"{base}[{idx}]"
        if nodetype == "Typename":
            return self.generate(node.get("type"))

        return f"/* Unsupported: {nodetype} */"

    def indent_str(self):
        return "    " * self.indent

    # ---------- 노드별 ----------

    def visit_file(self, node):
        out = []
        for item in node.get("ext", []):
            out.append(self.generate(item))
        return "\n".join(filter(None, out))

    def visit_funcdef(self, node):
        decl = self.generate(node.get("decl"))
        body = self.generate(node.get("body"))
        # decl 끝에 세미콜론이 붙을 수 있으니 제거
        if decl.endswith(";"):
            decl = decl[:-1]
        return f"{decl} {body}"

    def visit_decl(self, node):
        name = node.get("name", "")
        nod = node.get("type", {})
        # 함수 선언(프로토타입)인지, 일반 변수인지 구분
        if nod.get("_nodetype") == "FuncDecl":
            # 함수 프로토타입 형태
            proto = self.generate(nod)
            return f"{proto};"
        else:
            t = self.generate(nod)  # 예: "int x", "char *p" 등
            init_node = node.get("init")
            if init_node:
                init_str = self.generate(init_node)
                return f"{self.indent_str()}{t} = {init_str};"
            else:
                return f"{self.indent_str()}{t};"

    def visit_funcdecl(self, node):
        # 반환타입+이름
        t = self.generate(node.get("type", {}))  # "int main" 같은 문자열
        # 파라미터
        args = node.get("args")
        if not args:
            return f"{t}()"
        else:
            a = self.generate(args)  # ParamList
            return f"{t}({a})"

    def visit_paramlist(self, node):
        params = node.get("params", [])
        return ", ".join(self.generate(p) for p in params)

    def visit_typedecl(self, node):
        declname = node.get("declname", "")
        t = self.generate(node.get("type", {}))  # IdentifierType
        if declname:
            return f"{t} {declname}"
        return t

    def visit_ptrdecl(self, node):
        inner = self.generate(node.get("type", {}))  # "char x" 혹은 "char"
        # 단순히 '*' 붙이기
        # 실제 함수포인터 등은 여기서 복잡해짐
        return inner.replace("  ", " ") + " *"

    def visit_compound(self, node):
        items = node.get("block_items", [])
        out = ["{"]
        self.indent += 1
        for i in items or []:
            code = self.generate(i)
            if code:
                lines = code.split("\n")
                for ln in lines:
                    out.append(self.indent_str() + ln)
        self.indent -= 1
        out.append(self.indent_str() + "}")
        return "\n".join(out)

    def visit_return(self, node):
        expr = node.get("expr")
        if expr:
            e = self.generate(expr)
            return f"{self.indent_str()}return {e};"
        else:
            return f"{self.indent_str()}return;"

    def visit_if(self, node):
        cond = self.generate(node.get("cond"))
        iftrue = node.get("iftrue")
        iffalse = node.get("iffalse")

        out = []
        out.append(f"{self.indent_str()}if ({cond})")
        if iftrue:
            block = self.generate(iftrue)
            if block.startswith("{"):
                out.append(block)
            else:
                out.append(self.wrap_in_brace(block))
        if iffalse:
            out.append(f"{self.indent_str()}else")
            block = self.generate(iffalse)
            if block.startswith("{"):
                out.append(block)
            else:
                out.append(self.wrap_in_brace(block))
        return "\n".join(out)

    def visit_while(self, node):
        cond = self.generate(node.get("cond"))
        stmt = self.generate(node.get("stmt", {}))
        out = [f"{self.indent_str()}while ({cond})"]
        if stmt.startswith("{"):
            out.append(stmt)
        else:
            out.append(self.wrap_in_brace(stmt))
        return "\n".join(out)

    def visit_binop(self, node):
        left = self.generate(node.get("left"))
        right = self.generate(node.get("right"))
        op = node.get("op", "")
        return f"({left} {op} {right})"

    def visit_assign(self, node):
        l = self.generate(node.get("lvalue"))
        r = self.generate(node.get("rvalue"))
        op = node.get("op", "=")
        return f"{self.indent_str()}{l} {op} {r};"

    def visit_funccall(self, node):
        name = self.generate(node.get("name"))
        args = node.get("args")
        if args:
            a = self.generate(args)
            return f"{name}({a})"
        else:
            return f"{name}()"

    def wrap_in_brace(self, code):
        return "{\n" + self.indent_str() + "    " + code + "\n" + self.indent_str() + "}"


if __name__ == "__main__":
    # 예: ast.json 로드 후 코드 생성
    with open("ast.json", "r", encoding="utf-8") as f:
        ast_data = json.load(f)

    gen = CodeGen()
    result = gen.generate(ast_data)
    print(result)
