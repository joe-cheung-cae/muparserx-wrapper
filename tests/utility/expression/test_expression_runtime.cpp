#include "test_support.h"

#include "tfp/utility/expression/expression_error.h"
#include "tfp/utility/expression/expression_runtime.h"

#include <unordered_map>
#include <vector>

int main()
{
    using tfp::utility::ExpressionError;
    using tfp::utility::ExpressionRuntime;

    const char* config = R"json({
      "constants": {"rho0": "1000.0"},
      "tables": {
        "wind": {
          "extrapolation": "linear",
          "data": [[0.0, 0.0], [1.0, 2.0], [2.0, 4.0]]
        }
      },
      "expressions": {
        "fx": {"expression": "rho0 * wind(t)", "wordable": ["t"]},
        "sum_xy": {"expression": "x + 10.0 * y", "wordable": ["x", "y"]}
      }
    })json";

    ExpressionRuntime runtime;
    runtime.LoadFromJsonString(config);

    TFP_REQUIRE_NEAR(runtime.Evaluate("fx", std::unordered_map<std::string, double>{{"t", 1.5}}), 3000.0, 1e-12);
    TFP_REQUIRE_NEAR(runtime.GetUnaryExpression("fx").Evaluate(1.5), 3000.0, 1e-12);
    TFP_REQUIRE_NEAR(runtime.GetExpression("sum_xy").Evaluate(std::vector<double>{2.0, 3.0}), 32.0, 1e-12);

    TFP_REQUIRE_THROWS(ExpressionError, runtime.GetUnaryExpression("sum_xy"));
    TFP_REQUIRE_THROWS(ExpressionError, runtime.GetExpression("sum_xy").Evaluate(std::vector<double>{2.0}));
    TFP_REQUIRE_THROWS(ExpressionError, runtime.Evaluate("fx", std::unordered_map<std::string, double>()));
    TFP_REQUIRE_THROWS(ExpressionError, runtime.GetExpression("missing"));

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

    return 0;
}
