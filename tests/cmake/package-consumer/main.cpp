#include "tfp/utility/expression/expression.h"

#include <cmath>
#include <cstdlib>

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

int main()
{
    tfp::utility::ExpressionRuntime runtime =
        tfp::utility::ExpressionRuntime::CreateFromJsonString(R"json({
          "constants": {"rho0": "1000.0"},
          "tables": {
            "wind": {
              "extrapolation": "clamp",
              "data": [[0.0, 0.0], [1.0, 2.0]]
            }
          },
          "expressions": [
            {"name": "fx", "expression": "rho0 * wind(t)", "wordable": ["t"]}
          ]
        })json");

    Require(NearlyEqual(runtime.EvaluateUnary("rho0", 123.0), 1000.0, 1e-12));
    Require(NearlyEqual(runtime.EvaluateUnary("wind", 0.5), 1.0, 1e-12));
    Require(NearlyEqual(runtime.EvaluateUnary("fx", 0.5), 1000.0, 1e-12));

    return 0;
}
