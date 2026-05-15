#include "test_support.h"

#include "tfp/utility/expression/expression_config.h"
#include "tfp/utility/expression/expression_error.h"
#include "tfp/utility/expression/expression_runtime.h"

#include <type_traits>
#include <unordered_map>
#include <vector>

int main()
{
    using tfp::utility::ExpressionConfig;
    using tfp::utility::ExpressionError;
    using tfp::utility::ExpressionRuntime;
    using tfp::utility::ExpressionRuntimeConfig;
    using tfp::utility::ExtrapolationMode;
    using tfp::utility::TableConfig;

    static_assert(std::is_same<decltype(ExpressionRuntimeConfig::constants),
                               std::unordered_map<std::string, double> >::value,
                  "ExpressionRuntimeConfig constants must store resolved double values");

    const char* config = R"json({
      "constants": {"rho0": "1000.0"},
      "tables": {
        "wind": {
          "extrapolation": "linear",
          "data": [[0.0, 0.0], [1.0, 2.0], [2.0, 4.0]]
        }
      },
      "expressions": [
        {"name": "fx", "expression": "rho0 * wind(t)", "wordable": ["t"]},
        {"name": "sum_xy", "expression": "x + 10.0 * y", "wordable": ["x", "y"]}
      ]
    })json";

    ExpressionRuntime runtime;
    runtime.LoadFromJsonString(config);

    TFP_REQUIRE_NEAR(runtime.Evaluate("fx", std::unordered_map<std::string, double>{{"t", 1.5}}), 3000.0, 1e-12);
    TFP_REQUIRE_NEAR(runtime.EvaluateUnary("fx", 1.5), 3000.0, 1e-12);
    TFP_REQUIRE_NEAR(runtime.GetUnaryExpression("fx").Evaluate(1.5), 3000.0, 1e-12);
    TFP_REQUIRE_NEAR(runtime.GetExpression("sum_xy").Evaluate(std::vector<double>{2.0, 3.0}), 32.0, 1e-12);
    TFP_REQUIRE_NEAR(runtime.Evaluate("rho0", std::unordered_map<std::string, double>()), 1000.0, 1e-12);
    TFP_REQUIRE_NEAR(runtime.EvaluateUnary("rho0", 123.0), 1000.0, 1e-12);
    TFP_REQUIRE_NEAR(runtime.Evaluate("wind", std::unordered_map<std::string, double>{{"x", 1.5}}), 3.0, 1e-12);
    TFP_REQUIRE_NEAR(runtime.Evaluate("wind", std::unordered_map<std::string, double>{{"t", 1.5}}), 3.0, 1e-12);
    TFP_REQUIRE_NEAR(runtime.EvaluateUnary("wind", 1.5), 3.0, 1e-12);
    TFP_REQUIRE(runtime.GetArgumentNames("rho0").empty());
    TFP_REQUIRE(runtime.GetArgumentNames("wind") == std::vector<std::string>{"x"});
    TFP_REQUIRE(runtime.GetArgumentNames("fx") == std::vector<std::string>{"t"});
    TFP_REQUIRE(runtime.GetArgumentNames("sum_xy") == std::vector<std::string>({"x", "y"}));

    ExpressionRuntime factory_runtime = ExpressionRuntime::CreateFromJsonString(R"json({
      "functions": [
        {"name": "rho0", "function_type": 0, "value": "1000.0"},
        {
          "name": "wind",
          "function_type": 1,
          "extrapolation": "linear",
          "data": [[0.0, 0.0], [1.0, 2.0], [2.0, 4.0]]
        },
        {"name": "fx", "function_type": 2, "expression": "rho0 * wind(t)", "wordable": ["t"]},
        {"name": "sum_xy", "function_type": 2, "expression": "x + 10.0 * y", "wordable": ["x", "y"]}
      ]
    })json");

    TFP_REQUIRE_NEAR(factory_runtime.Evaluate("fx", std::unordered_map<std::string, double>{{"t", 1.5}}), 3000.0, 1e-12);
    TFP_REQUIRE_NEAR(factory_runtime.EvaluateUnary("fx", 1.5), 3000.0, 1e-12);
    TFP_REQUIRE_NEAR(factory_runtime.GetUnaryExpression("fx").Evaluate(1.5), 3000.0, 1e-12);
    TFP_REQUIRE_NEAR(factory_runtime.GetExpression("sum_xy").Evaluate(std::vector<double>{2.0, 3.0}), 32.0, 1e-12);
    TFP_REQUIRE_NEAR(factory_runtime.Evaluate("rho0", std::unordered_map<std::string, double>()), 1000.0, 1e-12);
    TFP_REQUIRE_NEAR(factory_runtime.EvaluateUnary("rho0", 123.0), 1000.0, 1e-12);
    TFP_REQUIRE_NEAR(factory_runtime.Evaluate("wind", std::unordered_map<std::string, double>{{"x", 1.5}}), 3.0, 1e-12);
    TFP_REQUIRE_NEAR(factory_runtime.Evaluate("wind", std::unordered_map<std::string, double>{{"t", 1.5}}), 3.0, 1e-12);
    TFP_REQUIRE_NEAR(factory_runtime.EvaluateUnary("wind", 1.5), 3.0, 1e-12);
    TFP_REQUIRE(factory_runtime.GetArgumentNames("rho0").empty());
    TFP_REQUIRE(factory_runtime.GetArgumentNames("wind") == std::vector<std::string>{"x"});
    TFP_REQUIRE(factory_runtime.GetArgumentNames("fx") == std::vector<std::string>{"t"});
    TFP_REQUIRE(factory_runtime.GetArgumentNames("sum_xy") == std::vector<std::string>({"x", "y"}));

    ExpressionRuntimeConfig object_config;
    object_config.constants["rho0"] = 1000.0;
    object_config.tables.push_back(TableConfig{"wind",
                                              std::vector<std::array<double, 2> >{{0.0, 0.0}, {1.0, 2.0}, {2.0, 4.0}},
                                              ExtrapolationMode::Linear});
    object_config.expressions.push_back(ExpressionConfig{"fx", "rho0 * wind(t)", std::vector<std::string>{"t"}});
    object_config.expressions.push_back(ExpressionConfig{"sum_xy", "x + 10.0 * y", std::vector<std::string>{"x", "y"}});

    ExpressionRuntime object_runtime;
    object_runtime.LoadFromConfig(object_config);

    TFP_REQUIRE_NEAR(object_runtime.Evaluate("fx", std::unordered_map<std::string, double>{{"t", 1.5}}), 3000.0, 1e-12);
    TFP_REQUIRE_NEAR(object_runtime.EvaluateUnary("fx", 1.5), 3000.0, 1e-12);
    TFP_REQUIRE_NEAR(object_runtime.GetUnaryExpression("fx").Evaluate(1.5), 3000.0, 1e-12);
    TFP_REQUIRE_NEAR(object_runtime.GetExpression("sum_xy").Evaluate(std::vector<double>{2.0, 3.0}), 32.0, 1e-12);
    TFP_REQUIRE_NEAR(object_runtime.Evaluate("rho0", std::unordered_map<std::string, double>()), 1000.0, 1e-12);
    TFP_REQUIRE_NEAR(object_runtime.EvaluateUnary("rho0", 123.0), 1000.0, 1e-12);
    TFP_REQUIRE_NEAR(object_runtime.Evaluate("wind", std::unordered_map<std::string, double>{{"x", 1.5}}), 3.0, 1e-12);
    TFP_REQUIRE_NEAR(object_runtime.Evaluate("wind", std::unordered_map<std::string, double>{{"t", 1.5}}), 3.0, 1e-12);
    TFP_REQUIRE_NEAR(object_runtime.EvaluateUnary("wind", 1.5), 3.0, 1e-12);
    TFP_REQUIRE(object_runtime.GetArgumentNames("rho0").empty());
    TFP_REQUIRE(object_runtime.GetArgumentNames("wind") == std::vector<std::string>{"x"});
    TFP_REQUIRE(object_runtime.GetArgumentNames("fx") == std::vector<std::string>{"t"});
    TFP_REQUIRE(object_runtime.GetArgumentNames("sum_xy") == std::vector<std::string>({"x", "y"}));

    ExpressionRuntime object_factory_runtime = ExpressionRuntime::CreateFromConfig(object_config);
    TFP_REQUIRE_NEAR(object_factory_runtime.Evaluate("fx", std::unordered_map<std::string, double>{{"t", 1.5}}), 3000.0, 1e-12);
    TFP_REQUIRE_NEAR(object_factory_runtime.EvaluateUnary("fx", 1.5), 3000.0, 1e-12);
    TFP_REQUIRE_NEAR(object_factory_runtime.GetUnaryExpression("fx").Evaluate(1.5), 3000.0, 1e-12);
    TFP_REQUIRE_NEAR(object_factory_runtime.GetExpression("sum_xy").Evaluate(std::vector<double>{2.0, 3.0}), 32.0, 1e-12);
    TFP_REQUIRE_NEAR(object_factory_runtime.Evaluate("rho0", std::unordered_map<std::string, double>()), 1000.0, 1e-12);
    TFP_REQUIRE_NEAR(object_factory_runtime.EvaluateUnary("rho0", 123.0), 1000.0, 1e-12);
    TFP_REQUIRE_NEAR(object_factory_runtime.Evaluate("wind", std::unordered_map<std::string, double>{{"x", 1.5}}), 3.0, 1e-12);
    TFP_REQUIRE_NEAR(object_factory_runtime.Evaluate("wind", std::unordered_map<std::string, double>{{"t", 1.5}}), 3.0, 1e-12);
    TFP_REQUIRE_NEAR(object_factory_runtime.EvaluateUnary("wind", 1.5), 3.0, 1e-12);
    TFP_REQUIRE(object_factory_runtime.GetArgumentNames("rho0").empty());
    TFP_REQUIRE(object_factory_runtime.GetArgumentNames("wind") == std::vector<std::string>{"x"});
    TFP_REQUIRE(object_factory_runtime.GetArgumentNames("fx") == std::vector<std::string>{"t"});
    TFP_REQUIRE(object_factory_runtime.GetArgumentNames("sum_xy") == std::vector<std::string>({"x", "y"}));

    ExpressionRuntimeConfig constant_item_config;
    constant_item_config.constants["magnetic_field"] = 0.05;
    ExpressionRuntime constant_item_runtime = ExpressionRuntime::CreateFromConfig(constant_item_config);
    TFP_REQUIRE(constant_item_runtime.GetArgumentNames("magnetic_field").empty());
    TFP_REQUIRE_NEAR(constant_item_runtime.Evaluate("magnetic_field", std::unordered_map<std::string, double>()),
                     0.05,
                     1e-12);
    TFP_REQUIRE_NEAR(constant_item_runtime.EvaluateUnary("magnetic_field", 123.0), 0.05, 1e-12);
    TFP_REQUIRE_THROWS_MESSAGE(ExpressionError,
                               constant_item_runtime.Evaluate("magnetic_field",
                                                              std::unordered_map<std::string, double>{{"x", 1.0}}),
                               "constant 'magnetic_field' expects no variables");

    ExpressionRuntimeConfig table_item_config;
    table_item_config.tables.push_back(TableConfig{
        "magnetic_field",
        std::vector<std::array<double, 2> >{{0.0, 0.0}, {1.0, 10.0}},
        ExtrapolationMode::Linear});
    ExpressionRuntime table_item_runtime = ExpressionRuntime::CreateFromConfig(table_item_config);
    TFP_REQUIRE(table_item_runtime.GetArgumentNames("magnetic_field") == std::vector<std::string>{"x"});
    TFP_REQUIRE_NEAR(table_item_runtime.Evaluate("magnetic_field",
                                                std::unordered_map<std::string, double>{{"x", 0.5}}),
                     5.0,
                     1e-12);
    TFP_REQUIRE_NEAR(table_item_runtime.Evaluate("magnetic_field",
                                                std::unordered_map<std::string, double>{{"t", 0.5}}),
                     5.0,
                     1e-12);
    TFP_REQUIRE_NEAR(table_item_runtime.EvaluateUnary("magnetic_field", 0.5), 5.0, 1e-12);
    TFP_REQUIRE_THROWS_MESSAGE(ExpressionError,
                               table_item_runtime.Evaluate("magnetic_field",
                                                           std::unordered_map<std::string, double>()),
                               "table 'magnetic_field' requires variable 'x'");

    ExpressionRuntimeConfig expression_item_config;
    expression_item_config.expressions.push_back(
        ExpressionConfig{"magnetic_field", "2.0 * t", std::vector<std::string>{"t"}});
    ExpressionRuntime expression_item_runtime = ExpressionRuntime::CreateFromConfig(expression_item_config);
    TFP_REQUIRE(expression_item_runtime.GetArgumentNames("magnetic_field") == std::vector<std::string>{"t"});
    TFP_REQUIRE_NEAR(expression_item_runtime.Evaluate("magnetic_field",
                                                     std::unordered_map<std::string, double>{{"t", 3.0}}),
                     6.0,
                     1e-12);
    TFP_REQUIRE_NEAR(expression_item_runtime.EvaluateUnary("magnetic_field", 3.0), 6.0, 1e-12);

    TFP_REQUIRE_THROWS(ExpressionError, runtime.GetUnaryExpression("sum_xy"));
    TFP_REQUIRE_THROWS_MESSAGE(ExpressionError, runtime.EvaluateUnary("sum_xy", 1.0), "expression 'sum_xy' is not unary");
    TFP_REQUIRE_THROWS(ExpressionError, runtime.GetExpression("sum_xy").Evaluate(std::vector<double>{2.0}));
    TFP_REQUIRE_THROWS(ExpressionError, runtime.Evaluate("fx", std::unordered_map<std::string, double>()));
    TFP_REQUIRE_THROWS_MESSAGE(ExpressionError,
                               runtime.Evaluate("rho0", std::unordered_map<std::string, double>{{"x", 1.0}}),
                               "constant 'rho0' expects no variables");
    TFP_REQUIRE_THROWS_MESSAGE(ExpressionError,
                               runtime.Evaluate("wind", std::unordered_map<std::string, double>()),
                               "table 'wind' requires variable 'x'");
    TFP_REQUIRE_THROWS_MESSAGE(ExpressionError,
                               runtime.Evaluate("wind", std::unordered_map<std::string, double>{{"t", 1.0}, {"y", 2.0}}),
                               "table 'wind' requires variable 'x'");
    TFP_REQUIRE_THROWS(ExpressionError, runtime.GetExpression("missing"));
    TFP_REQUIRE_THROWS(ExpressionError, runtime.GetArgumentNames("missing"));
    TFP_REQUIRE_THROWS_MESSAGE(ExpressionError,
                               runtime.Evaluate("unknown", std::unordered_map<std::string, double>()),
                               "runtime item 'unknown' does not exist");
    TFP_REQUIRE_THROWS_MESSAGE(ExpressionError,
                               runtime.EvaluateUnary("unknown", 0.0),
                               "runtime item 'unknown' does not exist");

    TFP_REQUIRE_THROWS(ExpressionError, ExpressionRuntime().LoadFromJsonString(R"json({
      "expressions": {"bad": {"expression": "t + z", "wordable": ["t"]}}
    })json"));

    TFP_REQUIRE_THROWS(ExpressionError, ExpressionRuntime().LoadFromJsonString(R"json({
      "expressions": {"bad": {"expression": "missing_table(t)", "wordable": ["t"]}}
    })json"));

    TFP_REQUIRE_THROWS(ExpressionError, ExpressionRuntime().LoadFromJsonString(R"json({
      "constants": {"dup": "1.0"},
      "expressions": {"dup": {"expression": "1.0", "wordable": []}}
    })json"));

    ExpressionRuntime table_runtime;
    table_runtime.LoadFromJsonString(R"json({
      "tables": {
        "wind": {
          "extrapolation": "error",
          "data": [[10.0, 1.0], [20.0, 2.0]]
        }
      },
      "expressions": {
        "wind_value": {"expression": "wind(t)", "wordable": ["t"]}
      }
    })json");
    TFP_REQUIRE_NEAR(table_runtime.Evaluate("wind_value", std::unordered_map<std::string, double>{{"t", 15.0}}), 1.5, 1e-12);
    TFP_REQUIRE_THROWS(ExpressionError,
                       table_runtime.Evaluate("wind_value", std::unordered_map<std::string, double>{{"t", 0.0}}));

    ExpressionRuntimeConfig invalid_config;
    invalid_config.constants["dup"] = 1.0;
    invalid_config.expressions.push_back(ExpressionConfig{"dup", "1.0", std::vector<std::string>()});
    TFP_REQUIRE_THROWS(ExpressionError, ExpressionRuntime().LoadFromConfig(invalid_config));

    ExpressionRuntimeConfig duplicate_wordable_config;
    duplicate_wordable_config.expressions.push_back(
        ExpressionConfig{"dup_wordable", "t", std::vector<std::string>{"t", "t"}});
    TFP_REQUIRE_THROWS(ExpressionError, ExpressionRuntime().LoadFromConfig(duplicate_wordable_config));

    return 0;
}
