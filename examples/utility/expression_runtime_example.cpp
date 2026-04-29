#include "tfp/utility/expression/expression_runtime.h"

#include <iostream>
#include <unordered_map>
#include <vector>

int main()
{
    const std::string config = R"json({
      "constants": {
        "rho0": "1000.0"
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
          "name": "dynamic_pressure",
          "expression": "0.5 * rho0 * wind(t)^2",
          "wordable": ["t"]
        },
        {
          "name": "weighted_sum",
          "expression": "x + 10.0 * y",
          "wordable": ["x", "y"]
        }
      ]
    })json";

    tfp::utility::ExpressionRuntime runtime;
    runtime.LoadFromJsonString(config);

    const tfp::utility::UnaryExpressionHandle fx = runtime.GetUnaryExpression("fx");
    std::cout << "fx(t) using unary handle\n";
    for (double t = 0.0; t <= 2.0; t += 0.5)
    {
        std::cout << "t = " << t << ", fx(t) = " << fx.Evaluate(t) << "\n";
    }

    const tfp::utility::ExpressionHandle weighted_sum = runtime.GetExpression("weighted_sum");
    const double weighted_value = weighted_sum.Evaluate(std::vector<double>{2.0, 3.0});
    std::cout << "\nweighted_sum(x=2, y=3) = " << weighted_value << "\n";

    const double dynamic_pressure = runtime.Evaluate(
        "dynamic_pressure",
        std::unordered_map<std::string, double>{{"t", 1.5}});
    std::cout << "dynamic_pressure(t=1.5) = " << dynamic_pressure << "\n";

    return 0;
}
