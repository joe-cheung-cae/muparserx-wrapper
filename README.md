# muparserx-wrapper

`muparserx-wrapper` is a C++17 JSON-defined expression runtime built on top of
muparserx. The wrapper consumes muparserx and nlohmann/json through
`find_package()` and keeps both dependency APIs out of the public headers.

The runtime is designed for the common pattern:

```cpp
tfp::utility::ExpressionRuntime runtime;
runtime.LoadFromJsonString(config);

auto fx = runtime.GetUnaryExpression("fx");
for (int i = 0; i < n; ++i)
{
    double value = fx.Evaluate(t);
}
```

JSON parsing, constant resolution, table validation/sorting, expression
compilation, variable registration, and table callback registration happen at
load time. Evaluation only updates already-bound variable slots and calls the
compiled parser.

## Features

- Loads constants from JSON.
- Resolves constant expressions at load time.
- Loads one-dimensional table functions from `[[x, y], ...]`.
- Supports linear interpolation with `clamp`, `error`, and `linear`
  extrapolation modes.
- Loads named expressions with ordered runtime variables (`wordable`).
- Allows expressions to call table functions.
- Provides fast unary and multi-argument expression handles.
- Provides a map-based convenience API for low-frequency/debug usage.
- Wraps backend errors in `tfp::utility::ExpressionError`.
- Keeps `mup::*` and `nlohmann::json` out of public headers.

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
```

## Dependencies

- CMake 3.14 or newer.
- A C++17 compiler.
- `muparserx`, discoverable through `find_package(muparserx CONFIG)`.
- `nlohmann/json`, discoverable through `find_package(nlohmann_json CONFIG)`.

## Clone

```bash
git clone <repo-url>
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

Run the example:

```bash
./build/expression_runtime_example
```

## CMake Integration

This project builds a library target named `tfp_expression`.

```cmake
find_package(muparserx REQUIRED CONFIG)
find_package(nlohmann_json REQUIRED CONFIG)
add_subdirectory(path/to/muparserx-wrapper)
target_link_libraries(your_target PRIVATE tfp_expression)
```

Include the public API:

```cpp
#include "tfp/utility/expression/expression_runtime.h"
```

## JSON Format

```json
{
  "constants": {
    "rho0": "1000.0",
    "pi_local": "3.141592653589793",
    "mu0": "4.0 * pi_local * 1e-7"
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

Tables are one-dimensional functions using `std::vector<std::array<double, 2>>`
internally. Each row is `[x, y]`.

Supported extrapolation modes:

- `"clamp"`: default; returns boundary y values outside the table range.
- `"error"`: throws on out-of-range input.
- `"linear"`: extends the first or last segment slope.

Table rows are sorted by `x` at load time. Duplicate `x` values and non-finite
values are errors.

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

runtime.LoadFromJsonString(json_text);
runtime.LoadFromJsonFile("config.json");

auto unary = runtime.GetUnaryExpression("fx");
double a = unary.Evaluate(1.5);

auto handle = runtime.GetExpression("weighted_sum");
double b = handle.Evaluate(std::vector<double>{2.0, 3.0});

double c = runtime.Evaluate("fx", {{"t", 1.5}});
```

Use `GetUnaryExpression()` for high-frequency one-argument expressions. Use
`GetExpression()` for high-frequency multi-argument expressions. Use the map API
for low-frequency or debug calls.

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
