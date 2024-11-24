step7 比較演算子まで

- 優先順位(低い順)

1. `==`, `!=`
2. `<`, `<=`, `>`. `>=`
3. `+`, `-`
4. `*`, `/`
5. `単項+`, `単項-`
6. `()`

- 文法

```
program = stmt*
stmt    = expr ";"
        | "return" expr ";"
expr       = assign
assign     = equality ("=" assign)?
equality   = relational ("==" relational | "!=" relational)*
relational = add ("<" add | "<=" add | ">" add | ">=" add)*
add        = mul ("+" mul | "-" mul)*
mul        = unary ("*" unary | "/" unary)*
unary      = ("+" | "-")? primary
primary    = num | ident | "(" expr ")"
```
