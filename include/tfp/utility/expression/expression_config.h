#ifndef TFP_UTILITY_EXPRESSION_CONFIG_H
#define TFP_UTILITY_EXPRESSION_CONFIG_H

#include "tfp/utility/expression/table_function.h"

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

namespace tfp
{
namespace utility
{

// TableConfig describes one named one-dimensional lookup table loaded into the
// runtime before expressions are compiled.
struct TableConfig
{
    std::string name;
    std::vector<std::array<double, 2> > data;
    ExtrapolationMode extrapolation = ExtrapolationMode::Clamp;
};

// ExpressionConfig describes one compiled expression entry. wordable defines
// the ordered runtime-variable list and therefore the positional argument order
// used by ExpressionHandle::Evaluate().
struct ExpressionConfig
{
    std::string name;
    std::string expression;
    std::vector<std::string> wordable;
};

// ExpressionRuntimeConfig is the normalized in-memory schema produced by the
// loader before constants, tables, and expressions are compiled.
struct ExpressionRuntimeConfig
{
    std::unordered_map<std::string, std::string> constants;
    std::vector<TableConfig> tables;
    std::vector<ExpressionConfig> expressions;
};

} // namespace utility
} // namespace tfp

#endif
