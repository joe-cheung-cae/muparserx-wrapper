你需要基于以下仓库进行开发：

https://github.com/joe-cheung-cae/muparserx.git

请 clone 该仓库，并在该仓库中初始化并实现一个 C++ 表达式运行时中间层库。该中间层用于封装当前仓库内的 muparserx，并提供 JSON-defined expression runtime。

============================================================
一、基础要求
============================================================

项目目标：
实现一个 JSON-defined expression runtime。它支持：

1. 从 JSON 读取 constants。
2. 从 JSON 读取一维 table 函数。
3. 从 JSON 读取命名 expressions。
4. expressions 可以调用 table 函数。
5. expressions 支持运行时变量 wordable。
6. 支持在 for-loop / 时间步循环中高频调用 Evaluate()。
7. 所有 JSON 解析、常量解析、table 校验排序、表达式编译、变量注册、函数注册都必须发生在加载/编译阶段。
8. Evaluate 阶段只能更新变量值并调用已经编译好的表达式求值。
9. Evaluate 阶段禁止重新解析 JSON。
10. Evaluate 阶段禁止重新 SetExpr。
11. Evaluate 阶段禁止重新注册 variables/constants/table functions。
12. Evaluate 阶段禁止重新排序 table。

底层表达式引擎：
使用当前仓库中的 muparserx 作为 private backend。

重要约束：
1. 当前仓库本身就是 muparserx fork，不要再额外下载另一个 muparserx。
2. 直接基于当前仓库中的 parser/ 目录实现 backend adapter。
3. public header 中绝对不能暴露任何 muparserx 类型。
4. mup::ParserX、mup::Value、mup::Variable、mup::ICallback、TokenPtr 等类型必须隔离在 backend/adapter 实现层。
5. 所有 muparserx 相关 include 只能出现在 src/ 实现文件或 private backend header 中。

C++ 标准：
使用 C++17 编译。

命名空间要求：
所有新增中间层代码必须放在：

namespace tfp
{
namespace utility
{

// code here

} // namespace utility
} // namespace tfp

禁止使用 C++17 风格的嵌套 namespace 写法，例如：

namespace tfp::utility
{
}

虽然工程可以使用 C++17 编译，但 namespace 写法必须保持传统形式，以保证代码风格和向下兼容性。

所有 public API、internal class、helper function、enum、error type 等，都应位于 tfp::utility 命名空间下。

例如：

tfp::utility::ExpressionRuntime
tfp::utility::ExpressionHandle
tfp::utility::UnaryExpressionHandle
tfp::utility::TableFunction
tfp::utility::ExpressionError
tfp::utility::ExtrapolationMode

第三方依赖：
1. muparserx：当前仓库已有，作为 private backend。
2. nlohmann/json：用于 JSON 解析。

不要在 public API 中暴露 nlohmann::json。JSON 解析应隔离在 loader 实现层。

============================================================
二、推荐工程目录结构
============================================================

请初始化如下目录结构：

include/
  tfp/utility/expression/
    expression_runtime.h
    expression_handle.h
    expression_error.h
    table_function.h
    expression_config.h

src/
  utility/expression/
    expression_runtime.cpp
    expression_handle.cpp
    expression_error.cpp
    table_function.cpp
    expression_config_loader.cpp
    constant_resolver.cpp
    expression_compiler.cpp
    muparserx_backend.cpp
    muparserx_table_callback.cpp

tests/
  utility/expression/
    test_table_function.cpp
    test_json_loader.cpp
    test_constant_resolver.cpp
    test_expression_runtime.cpp
    test_expression_evaluate_loop.cpp

examples/
  utility/
    expression_runtime_example.cpp

CMakeLists.txt

如果当前仓库已有构建风格，请尽量遵循当前风格。
如果没有，则使用清晰的现代 CMake 组织方式。

============================================================
三、核心 JSON Schema
============================================================

第一版固定支持如下 JSON 结构：

{
  "constants": {
    "rho0": "1000.0",
    "pi": "3.141592653589793",
    "mu0": "4.0 * pi * 1e-7"
  },

  "tables": {
    "wind": {
      "extrapolation": "clamp",
      "data": [
        [0.0, 0.0],
        [1.0, 2.5],
        [2.0, 4.0],
        [5.0, 3.0]
      ]
    }
  },

  "expressions": {
    "fx": {
      "expression": "rho0 * wind(t)",
      "wordable": ["t"]
    },

    "dynamic_pressure": {
      "expression": "0.5 * rho0 * wind(t)^2",
      "wordable": ["t"]
    }
  }
}

说明：

1. constants 是 name -> const expression。
2. const expression 是字符串，可以是纯数字，也可以引用其他 constants。
3. tables 是 name -> table definition。
4. table data 必须是 array-of-arrays 格式。
5. 每一行必须是 [x, y]。
6. table 内部数据模型必须简化为 std::vector<std::array<double, 2>>。
7. table 固定表示一维线性插值函数 f(x)。
8. expression 是普通表达式字符串，可以引用 constants、tables 和 wordable 中声明的运行时变量。
9. wordable 是该 expression 的运行时变量列表，同时也是高性能 Evaluate(args) 的参数顺序定义。
10. wordable 的顺序必须保留。

============================================================
四、Table Function 需求
============================================================

实现 TableFunction，语义如下：

TableFunction:
  - name
  - std::vector<std::array<double, 2>> data
  - extrapolation mode
  - Evaluate(double x) -> double

每个 row 的语义：

row[0] = x
row[1] = y

插值规则：

1. 如果 x 位于两个节点之间，使用线性插值。
2. 如果 x 正好等于某个节点的 x，返回该节点的 y。
3. 内部查找区间应使用二分查找，不要线性扫描。
4. TableFunction 构建完成后必须是 immutable/read-only，运行阶段不能修改 table 数据。

外推模式：

支持三种：

1. clamp：默认模式。小于 xmin 返回 y_min，大于 xmax 返回 y_max。
2. error：输入超出 table 范围时报错。
3. linear：使用首段或末段斜率做线性外推。

JSON 中 extrapolation 字段可选：

1. 如果缺失，默认 clamp。
2. 支持字符串："clamp"、"error"、"linear"。
3. 不支持的字符串必须报错。

Table 导入校验：

1. data 必须存在。
2. data 必须是 array。
3. data.size() >= 2。
4. data 中每一行必须是长度为 2 的 array。
5. 每个 x/y 都必须是数值。
6. 每个 x/y 都必须是 finite number，不允许 NaN/Inf。
7. 加载时检查 x 是否按升序排列。
8. 如果 x 未排序，必须自动按照 row[0] 升序重排序。
9. 重排序后检查是否存在重复 x。
10. 如果存在重复 x，必须报错。
11. 如果输入未排序但已自动重排，建议记录 warning，但不要中断加载。

不要引入以下复杂设计：

1. 不要引入 dataframe。
2. 不要引入列名。
3. 不要引入二维/三维 table。
4. 不要引入多输出 table。
5. 不要引入复杂 piecewise expression。
6. 不要引入自定义 point struct，除非内部实现确实有必要。
7. 对外和核心数据模型保持 std::vector<std::array<double, 2>>。

============================================================
五、Constants 需求
============================================================

constants 需要支持 const expression，而不只是 double。

示例：

{
  "constants": {
    "pi": "3.141592653589793",
    "rho0": "1000.0",
    "mu0": "4.0 * pi * 1e-7"
  }
}

常量解析规则：

1. 读取所有 raw constant expression。
2. 在加载阶段解析成 resolved constant values。
3. 常量表达式只能依赖其他常量。
4. 第一版不允许常量依赖 table function。
5. 第一版不允许常量依赖 runtime variable。
6. 如果常量表达式引用不存在的符号，必须报错。
7. 如果常量之间存在循环依赖，必须报错。
8. 常量解析完成后，resolved constants 是 name -> double。
9. 表达式编译时把 resolved constants 注册到 muparserx。

常量循环依赖示例：

{
  "constants": {
    "a": "b + 1",
    "b": "a + 1"
  }
}

这种必须在加载阶段报错。

实现建议：
可以用多轮解析或依赖图拓扑排序。
第一版只要行为正确，不要求极致性能。

============================================================
六、Expression 需求
============================================================

expressions JSON 结构：

"expressions": {
  "fx": {
    "expression": "rho0 * wind(t)",
    "wordable": ["t"]
  }
}

字段要求：

1. expression 字段必须存在。
2. expression 必须是 string。
3. wordable 字段必须存在。
4. wordable 必须是 string array。
5. wordable 中变量名不能重复。
6. wordable 的顺序必须保留。
7. wordable 顺序就是 Evaluate(args) 的参数顺序。

表达式编译阶段必须做：

1. 创建独立 backend/parser。
2. 注册所有 resolved constants。
3. 注册所有 table functions as muparserx callbacks。
4. 为 wordable 中每个变量创建持久的 variable storage。
5. 注册 wordable variables 到 muparserx。
6. SetExpr(expression)。
7. 预 Eval 一次，触发表达式语法检查和 RPN 构建。
8. 如果表达式引用了未定义符号，必须在加载/编译阶段报错。
9. 如果表达式引用了未声明到 wordable 的运行时变量，必须在加载/编译阶段报错。
10. 编译完成后得到 RuntimeExpression。

表达式语义：

1. 每个 expression 都是一个可重复求值的 runtime function。
2. 例如 fx.expression = "rho0 * wind(t)", fx.wordable = ["t"]，表示 fx(t)。
3. 运行时 Evaluate("fx", t) 应该返回 t 时刻对应的 fx 值，即 F(t)。

============================================================
七、Evaluate 高频调用需求
============================================================

这是本项目的核心要求。

运行典型模式：

runtime.LoadFromJson(config);

auto fx = runtime.GetUnaryExpression("fx");

for (...) {
    double t = ...;
    double value = fx.Evaluate(t);
}

必须满足：

1. LoadFromJson 之后，所有 expression 已经编译完成。
2. for-loop 中 Evaluate(t) 不允许重新解析 expression。
3. Evaluate(t) 不允许重新注册 constants。
4. Evaluate(t) 不允许重新注册 table function。
5. Evaluate(t) 不允许重新注册 variables。
6. Evaluate(t) 不允许重新排序 table。
7. Evaluate(t) 只允许更新已经绑定好的 variable storage，然后调用 parser.Eval()。
8. 这是必须遵守的性能约束。

需要提供三类 Evaluate 接口：

A. 高频一元表达式接口

用于 F(t) 场景。

概念接口：

auto handle = runtime.GetUnaryExpression("fx");
double value = handle.Evaluate(t);

要求：

1. 只有 wordable.size() == 1 的 expression 才能获取 UnaryExpressionHandle。
2. 如果 expression 不是一元表达式，GetUnaryExpression 必须报错。
3. UnaryExpressionHandle 内部缓存变量槽位。
4. UnaryExpressionHandle::Evaluate(double) 不应该做字符串查找。

B. 高频多参数表达式接口

用于 f(x, y, z) 场景。

概念接口：

auto handle = runtime.GetExpression("fxy");
double value = handle.Evaluate(args_in_wordable_order);

要求：

1. 参数顺序必须与 wordable 顺序一致。
2. args.size() 必须等于 wordable.size()。
3. Evaluate(args) 不应做变量名字查找，只按 slot 设置变量。
4. 如果 C++17 没有 std::span，可以使用 const std::vector<double>& 或自定义轻量 ArrayView。

C. 便捷 map 接口

用于调试和低频调用。

概念接口：

double value = runtime.Evaluate("fx", {{"t", 1.5}});

要求：

1. 可以有字符串查找。
2. 缺少 wordable 中变量必须报错。
3. 多余变量第一版建议 ignore。
4. 不推荐作为高频 for-loop 的主路径。

============================================================
八、Runtime 对象模型
============================================================

推荐内部对象关系：

ExpressionRuntime
  - ConstantRegistry
  - TableFunctionRegistry
  - ExpressionRegistry

ExpressionRegistry
  - name -> RuntimeExpression

RuntimeExpression
  - name
  - expression string
  - ordered wordable names
  - name-to-index map
  - backend/parser
  - persistent variable storage

TableFunctionRegistry
  - name -> shared_ptr<const TableFunction>

MuParserXBackend
  - private implementation only
  - owns mup::ParserX
  - owns variable storage or references RuntimeExpression variable storage
  - registers constants/tables/variables
  - provides EvalAsDouble()

重要约束：

1. public API 中不允许出现 muparserx 类型。
2. TableFunction 是中间层自主实现，不依赖 muparserx。
3. muparserx 只负责表达式解析和执行。
4. table function 通过 callback 注册到 muparserx，使表达式可以写 wind(t)、pressure(z)。

============================================================
九、线程安全策略
============================================================

第一版可以声明：

1. ExpressionRuntime / RuntimeExpression 的 Evaluate 不是线程安全的。
2. 因为 Evaluate 会更新绑定到 parser 的变量值。
3. TableFunction 是 immutable，可以跨线程共享。
4. 如果用户需要多线程并行 Evaluate，应为每个线程创建独立 runtime 或 expression clone。

架构预留：

1. 后续可以支持 CloneExpressionHandle 或 CreateThreadLocalInstance。
2. 第一版不需要实现复杂并发。

============================================================
十、错误处理
============================================================

不要把 muparserx 的异常直接暴露给上层。

定义自己的错误类型，例如：

1. ExpressionError
2. ConfigError
3. TableError
4. ConstantError
5. CompileError
6. EvaluationError

至少需要统一封装：

1. JSON schema 错误。
2. 常量解析错误。
3. 常量循环依赖。
4. table data 非法。
5. table duplicate x。
6. table extrapolation mode 非法。
7. expression 字段缺失。
8. wordable 字段非法。
9. expression 编译失败。
10. expression 运行失败。
11. Evaluate 参数数量错误。
12. Evaluate expression name 不存在。
13. GetUnaryExpression 用在非一元表达式上。

错误信息必须包含足够上下文，例如：

1. table name
2. expression name
3. constant name
4. variable name
5. JSON path if possible

============================================================
十一、命名冲突规则
============================================================

加载阶段需要检查全局符号冲突。

以下名称不应互相冲突：

1. constants
2. tables
3. expressions

runtime variable names 是 expression 局部的，由 wordable 管理。
第一版可以允许不同 expression 使用相同 wordable 变量名，例如多个表达式都使用 t。

但是：

1. constant 和 table 不能重名。
2. table 和 expression 不能重名。
3. constant 和 expression 不能重名。
4. 同一个 expression 的 wordable 不能重复。

如果发生冲突，加载阶段报错。

============================================================
十二、CMake 要求
============================================================

请建立现代 CMake 工程。

基本要求：

1. C++17。
2. 生成一个 library target，例如 tfp_expression。
3. public include 路径为 include/。
4. muparserx 作为 private dependency。
5. nlohmann/json 作为 private dependency，因为 public API 不应暴露 nlohmann::json。
6. tests 可选启用，例如 TFP_EXPRESSION_BUILD_TESTS。
7. examples 可选启用，例如 TFP_EXPRESSION_BUILD_EXAMPLES。
8. public header 不应该 include mpParser.h。
9. mpParser.h 只能出现在 src/backend 实现文件或 private header 中。

============================================================
十三、测试计划
============================================================

请实现单元测试，至少覆盖以下内容。

1. TableFunction 测试

- 正常线性插值。
- 输入刚好等于节点。
- clamp 左边界。
- clamp 右边界。
- linear 左外推。
- linear 右外推。
- error 模式越界报错。
- 输入 data 未排序时自动排序。
- 重复 x 报错。
- data 少于两个点报错。
- NaN/Inf 报错。

2. JSON Loader 测试

- 正常读取 constants/tables/expressions。
- table data 使用 [[x,y], ...] 格式。
- table data 行长度不是 2 报错。
- extrapolation 缺失时默认 clamp。
- extrapolation 非法字符串报错。
- expression 缺少 expression 字段报错。
- expression 缺少 wordable 字段报错。
- wordable 不是 array 报错。
- wordable 有重复变量报错。

3. ConstantResolver 测试

- 纯数字常量。
- 常量表达式依赖其他常量。
- 常量引用未知符号报错。
- 常量循环依赖报错。

4. ExpressionRuntime 测试

- expression 使用常量。
- expression 使用 table function。
- expression 使用 wordable 变量。
- expression 引用未声明 wordable 变量时报错。
- expression 引用不存在 table 报错。
- Evaluate("fx", map) 正常。
- GetUnaryExpression("fx").Evaluate(t) 正常。
- 非一元表达式调用 GetUnaryExpression 报错。
- 多参数表达式按 wordable 顺序 Evaluate(args) 正常。
- Evaluate(args) 参数数量错误时报错。

5. 高频循环测试

- 加载一次 JSON。
- 获取 unary handle。
- for 循环 1000 次或更多次调用 Evaluate(t)。
- 验证结果正确。
- 确保循环内没有重新 LoadFromJson、没有重新 Compile、没有重新 SetExpr。

============================================================
十四、示例程序
============================================================

examples/utility/expression_runtime_example.cpp 应展示以下流程：

1. 从 JSON string 或 JSON file 加载 runtime。
2. JSON 中包含：
   - constants: rho0
   - table: wind
   - expression: fx = rho0 * wind(t)
3. 获取 unary expression handle。
4. 在 for-loop 中改变 t。
5. 输出 t 和 fx(t)。

示例 JSON：

{
  "constants": {
    "rho0": "1000.0"
  },
  "tables": {
    "wind": {
      "extrapolation": "clamp",
      "data": [
        [0.0, 0.0],
        [1.0, 2.0],
        [2.0, 4.0]
      ]
    }
  },
  "expressions": {
    "fx": {
      "expression": "rho0 * wind(t)",
      "wordable": ["t"]
    }
  }
}

预期：

t = 1.5 时，wind(t) = 3.0，fx = 3000.0。

============================================================
十五、实现顺序
============================================================

请严格按照以下阶段实现，不要一次性把所有逻辑堆到一个大类中。

Phase 1: 工程初始化

- 建立 include/src/tests/examples 目录。
- 建立 CMakeLists.txt。
- 确认可以构建空库和示例。
- 确保新增代码位于 tfp::utility 命名空间。
- 禁止使用 namespace tfp::utility 写法。

Phase 2: TableFunction

- 实现 TableFunction。
- 实现 extrapolation mode。
- 实现排序、重复 x 检查、finite 检查。
- 实现单元测试。

Phase 3: JSON Config Loader

- 实现读取 constants/tables/expressions 的配置结构。
- 不做表达式编译，只做 schema parsing。
- table data 转为 std::vector<std::array<double, 2>>。
- 实现 JSON loader 测试。

Phase 4: ConstantResolver

- 支持 const expression 解析。
- 常量只允许依赖其他常量。
- 检查未知符号和循环依赖。
- 解析结果为 resolved constants。
- 实现测试。

Phase 5: MuParserXBackend

- 封装 muparserx。
- 注册 constants。
- 注册 variables。
- 注册 table functions callback。
- SetExpr + pre Eval。
- EvalAsDouble。
- 注意所有 muparserx 类型必须 private。

Phase 6: Expression Compiler / RuntimeExpression

- 实现 RuntimeExpression。
- 保存 wordable 顺序。
- 保存 name-to-index map。
- 变量 storage 持久化。
- 编译 expression。
- 预 Eval。
- 实现 Evaluate(args)。

Phase 7: ExpressionRuntime

- 实现 LoadFromJson。
- 按 constants -> tables -> expressions 顺序构建。
- 实现 Evaluate(name, map)。
- 实现 GetExpression(name)。
- 实现 GetUnaryExpression(name)。

Phase 8: Tests and Examples

- 补齐所有测试。
- 添加 example。
- 确保高频 Evaluate 测试通过。

============================================================
十六、关键架构原则
============================================================

请始终遵守以下原则：

1. Load once, compile once, evaluate many times.
2. Evaluate 阶段只能更新变量和执行 Eval。
3. TableFunction 是中间层自主实现。
4. TableFunction 数据模型固定为 std::vector<std::array<double, 2>>。
5. JSON table data 固定为 [[x, y], ...]。
6. wordable 是 expression 的有序运行时参数列表。
7. public API 不暴露 muparserx。
8. muparserx 只是 private backend。
9. 不要过度设计。
10. 不要引入 dataframe、多维 table、复杂 piecewise expression。
11. 第一版只支持 double scalar。
12. 先保证架构清晰、测试完整，再考虑性能微优化。
13. 所有新增代码使用 tfp::utility 命名空间。
14. 命名空间必须使用传统写法：

namespace tfp
{
namespace utility
{

} // namespace utility
} // namespace tfp

15. 禁止使用：

namespace tfp::utility
{
}s
