#include "test_support.h"

#include "tfp/utility/expression/expression_error.h"
#include "tfp/utility/expression/expression_runtime.h"

int main()
{
    using tfp::utility::ExpressionError;
    using tfp::utility::ExpressionRuntime;

    const char* valid_json = R"json({
      "constants": {"rho0": "1000.0"},
      "tables": {
        "wind": {
          "data": [[0.0, 0.0], [1.0, 2.0]]
        }
      },
      "expressions": {
        "fx": {"expression": "rho0 * wind(t)", "wordable": ["t"]}
      }
    })json";

    ExpressionRuntime runtime;
    runtime.LoadFromJsonString(valid_json);
    TFP_REQUIRE_NEAR(runtime.GetUnaryExpression("fx").Evaluate(0.5), 1000.0, 1e-12);

    TFP_REQUIRE_THROWS(ExpressionError, ExpressionRuntime().LoadFromJsonString(R"json({
      "tables": {"bad": {}},
      "expressions": {}
    })json"));

    TFP_REQUIRE_THROWS(ExpressionError, ExpressionRuntime().LoadFromJsonString(R"json({
      "tables": {"bad": {"data": "not-array"}},
      "expressions": {}
    })json"));

    TFP_REQUIRE_THROWS(ExpressionError, ExpressionRuntime().LoadFromJsonString(R"json({
      "tables": {"bad": {"data": [[0.0, 0.0], [1.0, 1e999]]}},
      "expressions": {}
    })json"));

    TFP_REQUIRE_THROWS(ExpressionError, ExpressionRuntime().LoadFromJsonString(R"json({
      "tables": {"bad": {"data": [[0.0, 0.0, 1.0], [1.0, 1.0]]}},
      "expressions": {}
    })json"));

    TFP_REQUIRE_THROWS(ExpressionError, ExpressionRuntime().LoadFromJsonString(R"json({
      "tables": {"bad": {"extrapolation": "nearest", "data": [[0.0, 0.0], [1.0, 1.0]]}},
      "expressions": {}
    })json"));

    TFP_REQUIRE_THROWS(ExpressionError, ExpressionRuntime().LoadFromJsonString(R"json({
      "expressions": {"fx": {"wordable": ["t"]}}
    })json"));

    TFP_REQUIRE_THROWS(ExpressionError, ExpressionRuntime().LoadFromJsonString(R"json({
      "expressions": {"fx": {"expression": "t"}}
    })json"));

    TFP_REQUIRE_THROWS(ExpressionError, ExpressionRuntime().LoadFromJsonString(R"json({
      "expressions": {"fx": {"expression": "t", "wordable": "t"}}
    })json"));

    TFP_REQUIRE_THROWS(ExpressionError, ExpressionRuntime().LoadFromJsonString(R"json({
      "expressions": {"fx": {"expression": "t", "wordable": ["t", "t"]}}
    })json"));

    return 0;
}
