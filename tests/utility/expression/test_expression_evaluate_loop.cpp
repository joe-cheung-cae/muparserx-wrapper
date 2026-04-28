#include "test_support.h"

#include "tfp/utility/expression/expression_runtime.h"

int main()
{
    tfp::utility::ExpressionRuntime runtime;
    runtime.LoadFromJsonString(R"json({
      "constants": {"rho0": "1000.0"},
      "tables": {
        "wind": {
          "extrapolation": "clamp",
          "data": [[0.0, 0.0], [1.0, 2.0], [2.0, 4.0]]
        }
      },
      "expressions": {
        "fx": {"expression": "rho0 * wind(t)", "wordable": ["t"]}
      }
    })json");

    const tfp::utility::UnaryExpressionHandle fx = runtime.GetUnaryExpression("fx");
    for (int i = 0; i < 1000; ++i)
    {
        const double t = static_cast<double>(i) / 1000.0;
        TFP_REQUIRE_NEAR(fx.Evaluate(t), 2000.0 * t, 1e-9);
    }

    return 0;
}
