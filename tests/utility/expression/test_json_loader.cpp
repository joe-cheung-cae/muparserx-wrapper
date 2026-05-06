#include "test_support.h"

#include "tfp/utility/expression/expression_error.h"
#include "tfp/utility/expression/expression_runtime.h"

#include <nlohmann/json.hpp>

int main()
{
    using tfp::utility::ExpressionError;
    using tfp::utility::ExpressionRuntime;

    const char* valid_json = R"json({
      "constants": {"rho0": "1000.0"},
      "tables": [
        {
          "name": "wind",
          "data": [[0.0, 0.0], [1.0, 2.0]]
        }
      ],
      "expressions": [
        {"name": "fx", "expression": "rho0 * wind(t)", "wordable": ["t"]}
      ]
    })json";

    ExpressionRuntime runtime;
    runtime.LoadFromJsonString(valid_json);
    TFP_REQUIRE_NEAR(runtime.GetUnaryExpression("fx").Evaluate(0.5), 1000.0, 1e-12);

    const nlohmann::json valid_json_object = nlohmann::json::parse(valid_json);

    ExpressionRuntime object_runtime;
    object_runtime.LoadFromJsonObject(valid_json_object);
    TFP_REQUIRE_NEAR(object_runtime.GetUnaryExpression("fx").Evaluate(0.5), 1000.0, 1e-12);

    ExpressionRuntime object_factory_runtime = ExpressionRuntime::CreateFromJsonObject(valid_json_object);
    TFP_REQUIRE_NEAR(object_factory_runtime.GetUnaryExpression("fx").Evaluate(0.5), 1000.0, 1e-12);

    TFP_REQUIRE_THROWS(ExpressionError, ExpressionRuntime().LoadFromJsonObject(nlohmann::json::array()));

    TFP_REQUIRE_THROWS(ExpressionError, ExpressionRuntime().LoadFromJsonObject(nlohmann::json{
        {"constants", nlohmann::json::array()}
    }));

    TFP_REQUIRE_THROWS(ExpressionError, ExpressionRuntime().LoadFromJsonObject(nlohmann::json{
        {"functions", nlohmann::json::array({
            {
                {"name", "bad"},
                {"function_type", 9223372036854775807ull},
                {"value", "1.0"}
            }
        })}
    }));

    ExpressionRuntime legacy_runtime;
    legacy_runtime.LoadFromJsonString(R"json({
      "tables": {
        "wind": {
          "data": [[0.0, 0.0], [1.0, 2.0]]
        }
      },
      "expressions": [
        {"name": "fx", "expression": "wind(t)", "wordable": ["t"]}
      ]
    })json");
    TFP_REQUIRE_NEAR(legacy_runtime.GetUnaryExpression("fx").Evaluate(0.5), 1.0, 1e-12);

    ExpressionRuntime factory_runtime;
    factory_runtime.LoadFromJsonString(R"json({
      "functions": [
        {"name": "rho0", "function_type": 0, "value": "1000.0"},
        {"name": "scale", "function_type": 0, "value": "2.0 * rho0"},
        {
          "name": "wind",
          "function_type": 1,
          "data": [[0.0, 0.0], [1.0, 2.0]]
        },
        {
          "name": "fx",
          "function_type": 2,
          "expression": "scale * wind(t)",
          "wordable": ["t"]
        }
      ]
    })json");
    TFP_REQUIRE_NEAR(factory_runtime.GetUnaryExpression("fx").Evaluate(0.5), 2000.0, 1e-12);

    TFP_REQUIRE_THROWS(ExpressionError, ExpressionRuntime().LoadFromJsonString(R"json({
      "functions": {}
    })json"));

    TFP_REQUIRE_THROWS(ExpressionError, ExpressionRuntime().LoadFromJsonString(R"json({
      "functions": [123]
    })json"));

    TFP_REQUIRE_THROWS(ExpressionError, ExpressionRuntime().LoadFromJsonString(R"json({
      "functions": [{"function_type": 0, "value": "1.0"}]
    })json"));

    TFP_REQUIRE_THROWS(ExpressionError, ExpressionRuntime().LoadFromJsonString(R"json({
      "functions": [{"name": "rho0", "value": "1.0"}]
    })json"));

    TFP_REQUIRE_THROWS(ExpressionError, ExpressionRuntime().LoadFromJsonString(R"json({
      "functions": [{"name": "rho0", "function_type": "0", "value": "1.0"}]
    })json"));

    TFP_REQUIRE_THROWS(ExpressionError, ExpressionRuntime().LoadFromJsonString(R"json({
      "functions": [{"name": "rho0", "function_type": 99, "value": "1.0"}]
    })json"));

    TFP_REQUIRE_THROWS(ExpressionError, ExpressionRuntime().LoadFromJsonString(R"json({
      "functions": [{"name": "rho0", "function_type": 0}]
    })json"));

    TFP_REQUIRE_THROWS(ExpressionError, ExpressionRuntime().LoadFromJsonString(R"json({
      "constants": {"rho0": "1000.0"},
      "functions": [{"name": "rho0", "function_type": 0, "value": "999.0"}]
    })json"));

    TFP_REQUIRE_THROWS(ExpressionError, ExpressionRuntime().LoadFromJsonString(R"json({
      "tables": [{"data": [[0.0, 0.0]]}],
      "expressions": {}
    })json"));

    TFP_REQUIRE_THROWS(ExpressionError, ExpressionRuntime().LoadFromJsonString(R"json({
      "tables": [{"name": 123, "data": [[0.0, 0.0]]}],
      "expressions": {}
    })json"));

    TFP_REQUIRE_THROWS(ExpressionError, ExpressionRuntime().LoadFromJsonString(R"json({
      "tables": ["bad"],
      "expressions": {}
    })json"));

    TFP_REQUIRE_THROWS(ExpressionError, ExpressionRuntime().LoadFromJsonString(R"json({
      "tables": "bad",
      "expressions": {}
    })json"));

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

    TFP_REQUIRE_THROWS(ExpressionError, ExpressionRuntime().LoadFromJsonString(R"json({
      "expressions": [
        {"expression": "t", "wordable": ["t"]}
      ]
    })json"));

    return 0;
}
