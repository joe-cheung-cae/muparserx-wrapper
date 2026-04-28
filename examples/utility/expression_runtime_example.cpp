#include "tfp/utility/expression/expression_runtime.h"

#include <iostream>

int main()
{
    const std::string config = R"json({
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
    })json";

    tfp::utility::ExpressionRuntime runtime;
    runtime.LoadFromJsonString(config);

    const tfp::utility::UnaryExpressionHandle fx = runtime.GetUnaryExpression("fx");
    for (double t = 0.0; t <= 2.0; t += 0.5)
    {
        std::cout << "t = " << t << ", fx(t) = " << fx.Evaluate(t) << "\n";
    }

    return 0;
}
