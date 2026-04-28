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

struct TableConfig
{
    std::string name;
    std::vector<std::array<double, 2> > data;
    ExtrapolationMode extrapolation = ExtrapolationMode::Clamp;
};

struct ExpressionConfig
{
    std::string name;
    std::string expression;
    std::vector<std::string> wordable;
};

struct ExpressionRuntimeConfig
{
    std::unordered_map<std::string, std::string> constants;
    std::vector<TableConfig> tables;
    std::vector<ExpressionConfig> expressions;
};

} // namespace utility
} // namespace tfp

#endif
