# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Common commands

This is a C++17 CMake project. It requires package configs for `muparserx` and `nlohmann_json`; pass their install prefixes through `CMAKE_PREFIX_PATH` when configuring a fresh build directory.

```bash
cmake -S . -B build-findpkg \
  -DCMAKE_PREFIX_PATH="/tmp/muparserx-install;/tmp/nlohmann-json-install" \
  -DTFP_EXPRESSION_BUILD_TESTS=ON \
  -DTFP_EXPRESSION_BUILD_EXAMPLES=ON
cmake --build build-findpkg --parallel
ctest --test-dir build-findpkg --output-on-failure
```

Run a single test with CTest name filtering:

```bash
ctest --test-dir build-findpkg -R '^test_expression_runtime$' --output-on-failure
```

Run the example program:

```bash
./build-findpkg/expression_runtime_example
```

The repository also contains a `build/` directory, but in this checkout it may fail to reconfigure if `muparserx` is not discoverable in that cache. Prefer `build-findpkg/` when using the local `/tmp/...-install` dependency prefixes.

There is no dedicated lint or formatting target in the current CMake configuration.

## Architecture overview

`muparserx-wrapper` builds the `tfp_expression` library, a JSON-defined expression runtime layered over muparserx. Public headers live under `include/tfp/utility/expression/` and intentionally avoid exposing muparserx or nlohmann/json types.

The main public entry point is `tfp::utility::ExpressionRuntime`. It loads JSON from a string or file, then returns either `ExpressionHandle` for ordered multi-argument evaluation or `UnaryExpressionHandle` for high-frequency one-variable evaluation. The map-based `Evaluate()` API is a convenience path for low-frequency/debug use.

The load pipeline is split across private source files:

- `expression_config_loader.cpp` parses JSON into internal `ExpressionRuntimeConfig` structs and validates shape/type constraints.
- `constant_resolver.cpp` evaluates string constants at load time, including dependencies between constants.
- `table_function.cpp` validates, sorts, and evaluates one-dimensional table functions with `clamp`, `error`, or `linear` extrapolation.
- `expression_runtime.cpp` checks global symbol-name conflicts, resolves constants, constructs tables, and compiles runtime expressions.
- `runtime_expression.cpp` owns each compiled expression's variable order, validates referenced variables, updates bound variable storage during evaluation, and wraps compile/evaluation failures.
- `muparserx_backend.cpp` and `muparserx_table_callback.cpp` isolate all direct muparserx integration.

JSON expressions use the `wordable` array as the ordered runtime-variable list. That order is the argument order for `ExpressionHandle::Evaluate(std::vector<double>)` and is also used to validate that expressions do not reference undeclared runtime variables.

Evaluation mutates parser-bound variable storage, so `ExpressionRuntime` and handles are not thread-safe for concurrent evaluation. Use one runtime instance per thread for parallel evaluation.

Tests are simple executable targets under `tests/utility/expression/`, one source file per CTest name. Add new tests by appending the target name to `TFP_EXPRESSION_TESTS` in `CMakeLists.txt` and creating the matching `tests/utility/expression/<name>.cpp` file.