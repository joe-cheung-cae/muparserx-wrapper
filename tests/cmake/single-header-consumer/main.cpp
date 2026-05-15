#include <tfp/utility/expression/expression_single_header.hpp>

#include <nlohmann/json.hpp>

#include <cmath>
#include <cstdlib>
#include <unordered_map>
#include <vector>

namespace
{

bool NearlyEqual(double lhs, double rhs, double epsilon)
{
    return std::fabs(lhs - rhs) <= epsilon;
}

void Require(bool condition)
{
    if (!condition)
    {
        std::exit(1);
    }
}

} // namespace

void TouchSingleHeaderSymbol()
{
    tfp::utility::ExpressionHandle handle;
    try
    {
        (void)handle.Arity();
        std::exit(1);
    }
    catch (const tfp::utility::ExpressionError&)
    {
    }
}

int main()
{
    using tfp::utility::ExpressionRuntime;

    const char* json_text = R"json({
      "constants": {"rho0": "1000.0", "scale": "2.0 * rho0"},
      "tables": {
        "wind": {
          "extrapolation": "linear",
          "data": [[0.0, 0.0], [1.0, 2.0], [2.0, 4.0]]
        }
      },
      "expressions": [
        {"name": "fx", "expression": "scale * wind(t)", "wordable": ["t"]},
        {"name": "sum_xy", "expression": "x + 10.0 * y", "wordable": ["x", "y"]}
      ]
    })json";

    ExpressionRuntime runtime = ExpressionRuntime::CreateFromJsonString(json_text);
    Require(NearlyEqual(runtime.GetUnaryExpression("fx").Evaluate(1.5), 6000.0, 1e-12));
    Require(NearlyEqual(runtime.Evaluate("fx", std::unordered_map<std::string, double>{{"t", 1.5}}), 6000.0, 1e-12));
    Require(NearlyEqual(runtime.EvaluateUnary("scale", 123.0), 2000.0, 1e-12));
    Require(NearlyEqual(runtime.EvaluateUnary("wind", 1.5), 3.0, 1e-12));
    Require(NearlyEqual(runtime.EvaluateUnary("fx", 1.5), 6000.0, 1e-12));
    Require(NearlyEqual(runtime.GetExpression("sum_xy").Evaluate(std::vector<double>{2.0, 3.0}), 32.0, 1e-12));

    const nlohmann::json json_object = nlohmann::json::parse(json_text);
    ExpressionRuntime object_runtime = ExpressionRuntime::CreateFromJsonObject(json_object);
    Require(NearlyEqual(object_runtime.GetUnaryExpression("fx").Evaluate(-1.0), -4000.0, 1e-12));
    Require(NearlyEqual(object_runtime.EvaluateUnary("wind", -1.0), -2.0, 1e-12));

    TouchSingleHeaderSymbol();
    return 0;
}
