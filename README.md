# muparserx-wrapper

`muparserx-wrapper` is a C++17 JSON-defined expression runtime built on top of
muparserx. The wrapper consumes muparserx and nlohmann/json through
`find_package()`. The public runtime API exposes `nlohmann::json` for callers
that already have parsed JSON objects, while muparserx stays behind the wrapper.

The runtime supports both the legacy section-based JSON schema
(`constants` / `tables` / `expressions`) and a factory-style `functions` schema
with integral `function_type` values (`0` constant, `1` table, `2` expression-like).

The runtime is designed for the common expression pattern:

```cpp
tfp::utility::ExpressionRuntime runtime;
runtime.LoadFromJsonString(config);

auto fx = runtime.GetUnaryExpression("fx");
for (int i = 0; i < n; ++i)
{
    double value = fx.Evaluate(t);
}
```

If a one-dimensional runtime item may be configured as a constant, table, or
expression, call it through the unified unary interface:

```cpp
double scale = runtime.EvaluateUnary("magnetic_field", time);
```

`EvaluateUnary()` ignores `time` for constants, evaluates tables at `time`, and
requires expressions to declare exactly one runtime variable. Use
`GetArgumentNames()` when callers need to inspect the loaded shape explicitly:

```cpp
const std::vector<std::string> args = runtime.GetArgumentNames("magnetic_field");
```

JSON parsing, constant resolution, table validation/sorting, expression
compilation, variable registration, and table callback registration happen at
load time. Evaluation only updates already-bound variable slots and calls the
compiled parser.

## Features

- Loads constants from JSON.
- Also supports programmatic loading from parsed `nlohmann::json` objects and
  `ExpressionRuntimeConfig`.
- Resolves constant expressions at load time.
- Loads one-dimensional table functions from `[[x, y], ...]`.
- Supports linear interpolation with `clamp`, `error`, and `linear`
  extrapolation modes.
- Loads named expressions with ordered runtime variables (`wordable`).
- Allows expressions to call table functions.
- Provides fast unary and multi-argument expression handles.
- Exposes ordered runtime argument names for constants, tables, and expressions.
- Evaluates constants, tables, and expressions through a unified runtime-item
  API.
- Provides a map-based convenience API for low-frequency/debug usage.
- Wraps backend errors in `tfp::utility::ExpressionError`.
- Keeps `mup::*` out of public headers.

## Repository Layout

```text
include/tfp/utility/expression/
  expression_runtime.h
  expression_handle.h
  expression_error.h
  table_function.h
  expression_config.h

src/utility/expression/
  expression_runtime.cpp
  expression_handle.cpp
  expression_error.cpp
  table_function.cpp
  expression_config_loader.cpp
  constant_resolver.cpp
  expression_compiler.cpp
  muparserx_backend.cpp
  muparserx_table_callback.cpp

tests/utility/expression/
  test_table_function.cpp
  test_json_loader.cpp
  test_constant_resolver.cpp
  test_expression_runtime.cpp
  test_expression_evaluate_loop.cpp

examples/utility/
  expression_runtime_example.cpp
  expression_single_header_example.cpp
```

## Dependencies

- CMake 3.14 or newer.
- A C++17 compiler.
- `muparserx`, discoverable through `find_package(muparserx CONFIG)`.
- `nlohmann/json`, discoverable through `find_package(nlohmann_json CONFIG)`.

## Clone

```bash
git clone https://github.com/joe-cheung-cae/muparserx-wrapper.git
cd muparserx-wrapper
```

## Dependency Setup

Install or otherwise provide CMake package configs for both dependencies before
configuring this project.

For muparserx, CMake must be able to find `muparserxConfig.cmake`, and that
package must provide:

- `muparserx_INCLUDE_DIRS`
- `muparserx_LIBRARIES`

For nlohmann/json, CMake must be able to find `nlohmann_jsonConfig.cmake`, which
provides the imported target `nlohmann_json::nlohmann_json`.

If the dependencies are installed into non-standard prefixes, pass them through
`CMAKE_PREFIX_PATH`.

## Build

```bash
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH="/path/to/muparserx;/path/to/nlohmann-json" \
  -DTFP_EXPRESSION_BUILD_TESTS=ON \
  -DTFP_EXPRESSION_BUILD_EXAMPLES=ON

cmake --build build --parallel
```

Run tests:

```bash
ctest --test-dir build --output-on-failure
```

Run a single test:

```bash
ctest --test-dir build -R '^test_expression_runtime$' --output-on-failure
```

Run the examples:

```bash
./build/expression_runtime_example
./build/expression_single_header_example
```

## CMake Integration

This project builds the `tfp_expression` target and exports the namespaced alias `tfp::expression`.

Consume it directly from source:

```cmake
find_package(muparserx REQUIRED CONFIG)
find_package(nlohmann_json REQUIRED CONFIG)
add_subdirectory(path/to/muparserx-wrapper)
target_link_libraries(your_target PRIVATE tfp::expression)
```

When used as a subdirectory, tests and examples are disabled by default unless the parent project enables `TFP_EXPRESSION_BUILD_TESTS` or `TFP_EXPRESSION_BUILD_EXAMPLES`.

Install and consume it as a package:

```bash
cmake -S . -B build-install \
  -DCMAKE_PREFIX_PATH="/path/to/muparserx;/path/to/nlohmann-json" \
  -DCMAKE_INSTALL_PREFIX=/tmp/tfp-expression-install
cmake --build build-install --parallel
cmake --install build-install
```

```cmake
find_package(tfpExpression CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE tfp::expression)
```

Include the public API:

```cpp
#include "tfp/utility/expression/expression.h"
```

## Single-header Usage

A header-only snapshot is available for embedding the wrapper directly into another project, and `examples/utility/expression_single_header_example.cpp` shows the in-repo usage pattern:

```cpp
#include <tfp/utility/expression/expression_single_header.hpp>
```

Add `single_include/` to your include path. This mode does not link `tfp_expression`, but it still requires C++17, `nlohmann/json.hpp`, muparserx headers, and the muparserx library.

```cmake
find_package(muparserx REQUIRED CONFIG)
find_package(nlohmann_json 3.11.0 REQUIRED CONFIG)

add_executable(your_target main.cpp)
target_compile_features(your_target PRIVATE cxx_std_17)
target_include_directories(your_target PRIVATE
  /path/to/muparserx-wrapper/single_include
  ${muparserx_INCLUDE_DIRS}
)
target_link_libraries(your_target PRIVATE
  ${muparserx_LIBRARIES}
  nlohmann_json::nlohmann_json
)
```

On Windows, the wrapper itself needs no `__declspec(dllexport)` or `__declspec(dllimport)` in single-header mode because it is compiled into the consuming target. Link muparserx according to your muparserx static or DLL installation, and define `NOMINMAX` in your application if your Windows headers require it.

## JSON Format

### Factory-style `functions` schema

```json
{
  "functions": [
    {
      "name": "rho0",
      "function_type": 0,
      "value": "1000.0"
    },
    {
      "name": "scale",
      "function_type": 0,
      "value": "2.0 * rho0"
    },
    {
      "name": "wind",
      "function_type": 1,
      "extrapolation": "clamp",
      "data": [
        [0.0, 0.0],
        [1.0, 2.0],
        [2.0, 4.0]
      ]
    },
    {
      "name": "fx",
      "function_type": 2,
      "expression": "scale * wind(t)",
      "wordable": ["t"]
    },
    {
      "name": "weighted_sum",
      "function_type": 2,
      "expression": "x + 10.0 * y",
      "wordable": ["x", "y"]
    }
  ]
}
```

`function_type` values are:

- `0`: constant
- `1`: table
- `2`: expression-like

Factory-style constants use `value`, tables use `data` plus optional
`extrapolation`, and expression-like functions use `expression` plus
`wordable`.

The loader still supports the legacy section-based schema below.

At runtime, the public API treats these categories uniformly when querying
argument names:

- constants -> `GetArgumentNames(name)` returns `{}`
- tables -> returns `{"x"}`
- expressions -> returns the declared `wordable` order

The runtime evaluation APIs also support these categories uniformly:

- `Evaluate(name, variables)` evaluates expressions with the supplied variable
  map, tables with variable `"x"` or a single supplied variable value, and
  constants with an empty variable map.
- `EvaluateUnary(name, x)` evaluates one-dimensional runtime items directly:
  constants ignore `x`, tables evaluate at `x`, and expressions must declare
  exactly one runtime variable.

### Legacy section-based schema

```json
{
  "constants": {
    "rho0": "1000.0",
    "pi_local": "3.141592653589793",
    "mu0": "4.0 * pi_local * 1e-7"
  },
  "tables": [
    {
      "name": "wind",
      "extrapolation": "clamp",
      "data": [
        [0.0, 0.0],
        [1.0, 2.0],
        [2.0, 4.0]
      ]
    }
  ],
  "expressions": [
    {
      "name": "fx",
      "expression": "rho0 * wind(t)",
      "wordable": ["t"]
    },
    {
      "name": "weighted_sum",
      "expression": "x + 10.0 * y",
      "wordable": ["x", "y"]
    }
  ]
}
```

### Constants

Constants are string expressions. They are resolved to `double` values during
load:

```json
"constants": {
  "pi_local": "3.141592653589793",
  "mu0": "4.0 * pi_local * 1e-7"
}
```

Constants may depend on other constants. They may not depend on runtime
variables or table functions. Unknown symbols and dependency cycles are errors.

### Tables

`tables` is an array of table objects. Each table object has:

- `name`: the runtime lookup name.
- `extrapolation`: optional; one of `"clamp"`, `"error"`, or `"linear"`.
- `data`: an array of `[x, y]` rows.

Tables are one-dimensional functions using `std::vector<std::array<double, 2>>`
internally. Each row is `[x, y]`.

Supported extrapolation modes:

- `"clamp"`: default; returns boundary y values outside the table range.
- `"error"`: throws on out-of-range input.
- `"linear"`: extends the first or last segment slope.

Table rows are sorted by `x` at load time. Duplicate `x` values and non-finite
values are errors.

For compatibility, the loader also accepts the older object form:

```json
"tables": {
  "wind": {
    "extrapolation": "clamp",
    "data": [[0.0, 0.0], [1.0, 2.0]]
  }
}
```

### Expressions

`expressions` is an array. Each expression object has:

- `name`: the runtime lookup name.
- `expression`: a muparserx expression string.
- `wordable`: ordered runtime variable names.

The `wordable` order is the argument order for fast evaluation.

For compatibility, the loader also accepts the older object form:

```json
"expressions": {
  "fx": {
    "expression": "rho0 * wind(t)",
    "wordable": ["t"]
  }
}
```

## Public API

```cpp
tfp::utility::ExpressionRuntime runtime;

auto from_json = tfp::utility::ExpressionRuntime::CreateFromJsonString(json_text);
auto from_object = tfp::utility::ExpressionRuntime::CreateFromJsonObject(json_object);
auto from_file = tfp::utility::ExpressionRuntime::CreateFromJsonFile("config.json");
auto from_config = tfp::utility::ExpressionRuntime::CreateFromConfig(config_object);

runtime.LoadFromJsonString(json_text);
runtime.LoadFromJsonObject(json_object);
runtime.LoadFromJsonFile("config.json");
runtime.LoadFromConfig(config_object);

auto unary = runtime.GetUnaryExpression("fx");
double a = unary.Evaluate(1.5);

auto handle = runtime.GetExpression("weighted_sum");
double b = handle.Evaluate(std::vector<double>{2.0, 3.0});

double c = runtime.Evaluate("fx", {{"t", 1.5}});
double d = runtime.EvaluateUnary("magnetic_field", time);
```

Use `LoadFromJsonObject()` when your application already owns a parsed
`nlohmann::json` object. Use `LoadFromConfig()` when constructing the normalized
runtime model in C++. Use `GetUnaryExpression()` for high-frequency one-argument
expressions. Use `GetExpression()` for high-frequency multi-argument
expressions. Use `EvaluateUnary()` when solver code needs one call site that
works whether a loaded item is a constant, table, or unary expression. Use the
map API for low-frequency or debug calls.

## Error Handling

The wrapper throws `tfp::utility::ExpressionError` for configuration, compile,
and evaluation failures.

```cpp
try
{
    runtime.LoadFromJsonString(json_text);
}
catch (const tfp::utility::ExpressionError& error)
{
    std::cerr << error.what() << std::endl;
}
```

muparserx exceptions are converted at the wrapper boundary.

## Thread Safety

`ExpressionRuntime` and expression handles are not thread-safe for concurrent
evaluation because `Evaluate()` updates parser-bound variable storage.

Use one runtime instance per thread for parallel evaluation. `TableFunction`
objects are immutable after construction and may be shared internally.

## Example Output

`examples/utility/expression_runtime_example.cpp` demonstrates:

- Loading a JSON config string.
- Loading a parsed `nlohmann::json` object.
- Evaluating a unary expression in a loop.
- Evaluating a multi-argument expression by ordered arguments.
- Evaluating through the map-based convenience API.

Expected output:

```text
fx(t) using unary handle
t = 0, fx(t) = 0
t = 0.5, fx(t) = 1000
t = 1, fx(t) = 2000
t = 1.5, fx(t) = 3000
t = 2, fx(t) = 4000

weighted_sum(x=2, y=3) = 32
dynamic_pressure(t=1.5) = 4500
```
