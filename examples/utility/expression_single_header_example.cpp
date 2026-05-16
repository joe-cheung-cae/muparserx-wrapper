#include <tfp/utility/expression/expression_single_header.hpp>

#include <iostream>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <vector>

int main()
{
    const std::string config = R"json({
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
          "name": "magnetic_field",
          "function_type": 1,
          "extrapolation": "clamp",
          "data": [
            [0.0, 0.0],
            [1.0, 10.0]
          ]
        },
        {
          "name": "fx",
          "function_type": 2,
          "expression": "scale * wind(t)",
          "wordable": ["t"]
        },
        {
          "name": "dynamic_pressure",
          "function_type": 2,
          "expression": "0.5 * rho0 * wind(t)^2",
          "wordable": ["t"]
        },
        {
          "name": "weighted_sum",
          "function_type": 2,
          "expression": "x + 10.0 * y",
          "wordable": ["x", "y"]
        }
      ]
    })json";

    tfp::utility::ExpressionRuntime runtime =
        tfp::utility::ExpressionRuntime::CreateFromJsonString(config);

    // The single-header API preserves the same unary fast path as the library build.
    const tfp::utility::UnaryExpressionHandle fx = runtime.GetUnaryExpression("fx");
    std::cout << "fx(t) using single-header unary handle\n";
    for (double t = 0.0; t <= 2.0; t += 0.5)
    {
        std::cout << "t = " << t << ", fx(t) = " << fx.Evaluate(t) << "\n";
    }

    // Multi-argument expressions still use the ordered positional handle.
    const tfp::utility::ExpressionHandle weighted_sum = runtime.GetExpression("weighted_sum");
    const double weighted_value = weighted_sum.Evaluate(std::vector<double>{2.0, 3.0});
    std::cout << "\nweighted_sum(x=2, y=3) = " << weighted_value << "\n";

    // Evaluate() is convenient for low-frequency named-variable calls.
    const double dynamic_pressure = runtime.Evaluate(
        "dynamic_pressure",
        std::unordered_map<std::string, double>{{"t", 1.5}});
    std::cout << "dynamic_pressure(t=1.5) = " << dynamic_pressure << "\n";

    const double magnetic_field = runtime.EvaluateUnary("magnetic_field", 0.5);
    std::cout << "magnetic_field(time=0.5) = " << magnetic_field << "\n";

    const double density = runtime.EvaluateUnary("rho0", 123.0);
    std::cout << "rho0 through EvaluateUnary = " << density << "\n";

    const nlohmann::json config_object = nlohmann::json::parse(config);
    tfp::utility::ExpressionRuntime object_runtime =
        tfp::utility::ExpressionRuntime::CreateFromJsonObject(config_object);

    std::cout << "fx(t=1.5) from parsed JSON object = "
              << object_runtime.GetUnaryExpression("fx").Evaluate(1.5) << "\n";

    return 0;
}
