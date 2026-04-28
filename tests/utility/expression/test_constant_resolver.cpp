#include "test_support.h"

#include "tfp/utility/expression/expression_error.h"
#include "tfp/utility/expression/expression_runtime.h"

int main()
{
    using tfp::utility::ExpressionError;
    using tfp::utility::ExpressionRuntime;

    ExpressionRuntime runtime;
    runtime.LoadFromJsonString(R"json({
      "constants": {
        "pi_local": "3.141592653589793",
        "rho0": "1000.0",
        "mu0": "4.0 * pi_local * 1e-7"
      },
      "expressions": {
        "mu": {"expression": "mu0", "wordable": []}
      }
    })json");
    TFP_REQUIRE_NEAR(runtime.GetExpression("mu").Evaluate(std::vector<double>()), 1.2566370614359173e-6, 1e-18);

    TFP_REQUIRE_THROWS(ExpressionError, ExpressionRuntime().LoadFromJsonString(R"json({
      "constants": {"a": "missing + 1.0"},
      "expressions": {}
    })json"));

    TFP_REQUIRE_THROWS(ExpressionError, ExpressionRuntime().LoadFromJsonString(R"json({
      "constants": {"a": "b + 1.0", "b": "a + 1.0"},
      "expressions": {}
    })json"));

    return 0;
}
