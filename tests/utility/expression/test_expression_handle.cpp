#include "tfp/utility/expression/expression_error.h"
#include "tfp/utility/expression/expression_handle.h"

#include "test_support.h"

#include <vector>

int main()
{
    tfp::utility::ExpressionHandle handle;
    tfp::utility::UnaryExpressionHandle unary_handle;

    TFP_REQUIRE_THROWS(tfp::utility::ExpressionError, handle.Evaluate(std::vector<double>{1.0}));
    TFP_REQUIRE_THROWS(tfp::utility::ExpressionError, handle.Arity());
    TFP_REQUIRE_THROWS(tfp::utility::ExpressionError, unary_handle.Evaluate(1.0));

    return 0;
}
