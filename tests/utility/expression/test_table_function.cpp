#include "test_support.h"

#include "tfp/utility/expression/expression_error.h"
#include "tfp/utility/expression/table_function.h"

#include <array>
#include <limits>
#include <vector>

int main()
{
    using tfp::utility::ExpressionError;
    using tfp::utility::ExtrapolationMode;
    using tfp::utility::TableFunction;

    TableFunction clamp_table(
        "wind",
        std::vector<std::array<double, 2> >{{2.0, 4.0}, {0.0, 0.0}, {1.0, 2.0}},
        ExtrapolationMode::Clamp);

    TFP_REQUIRE_NEAR(clamp_table.Evaluate(0.5), 1.0, 1e-12);
    TFP_REQUIRE_NEAR(clamp_table.Evaluate(1.0), 2.0, 1e-12);
    TFP_REQUIRE_NEAR(clamp_table.Evaluate(-1.0), 0.0, 1e-12);
    TFP_REQUIRE_NEAR(clamp_table.Evaluate(3.0), 4.0, 1e-12);

    TableFunction linear_table(
        "wind",
        std::vector<std::array<double, 2> >{{0.0, 0.0}, {1.0, 2.0}, {2.0, 4.0}},
        ExtrapolationMode::Linear);
    TFP_REQUIRE_NEAR(linear_table.Evaluate(-1.0), -2.0, 1e-12);
    TFP_REQUIRE_NEAR(linear_table.Evaluate(3.0), 6.0, 1e-12);

    TableFunction error_table(
        "wind",
        std::vector<std::array<double, 2> >{{0.0, 0.0}, {1.0, 2.0}},
        ExtrapolationMode::Error);
    TFP_REQUIRE_THROWS(ExpressionError, error_table.Evaluate(-0.1));
    TFP_REQUIRE_THROWS(ExpressionError, error_table.Evaluate(1.1));

    TFP_REQUIRE_THROWS(
        ExpressionError,
        TableFunction("dup", std::vector<std::array<double, 2> >{{0.0, 0.0}, {0.0, 1.0}},
                      ExtrapolationMode::Clamp));
    TFP_REQUIRE_THROWS(
        ExpressionError,
        TableFunction("small", std::vector<std::array<double, 2> >{{0.0, 0.0}},
                      ExtrapolationMode::Clamp));
    TFP_REQUIRE_THROWS(
        ExpressionError,
        TableFunction("bad", std::vector<std::array<double, 2> >{{0.0, 0.0}, {1.0, std::numeric_limits<double>::infinity()}},
                      ExtrapolationMode::Clamp));

    return 0;
}
