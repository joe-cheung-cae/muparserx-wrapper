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

/// @brief In-memory definition of one named one-dimensional lookup table.
struct TableConfig
{
    /// Symbol name used when expressions call the table as a function.
    std::string name;

    /// Table rows expressed as [x, y] pairs.
    ///
    /// Runtime loading validates finite values, sorts rows by x, and rejects
    /// duplicate x coordinates.
    std::vector<std::array<double, 2> > data;

    /// Boundary policy used when evaluating outside the configured x range.
    ExtrapolationMode extrapolation = ExtrapolationMode::Clamp;
};

/// @brief In-memory definition of one compiled expression entry.
///
/// The @c wordable field defines the ordered runtime-variable list and
/// therefore the positional argument order used by ExpressionHandle::Evaluate().
struct ExpressionConfig
{
    /// Unique expression symbol name used for runtime lookup.
    std::string name;

    /// Parser expression text.
    ///
    /// The expression may refer to constants, tables, and the runtime
    /// variables listed in @c wordable.
    std::string expression;

    /// Ordered runtime-variable list.
    ///
    /// Names must be unique within the expression, and every runtime variable
    /// referenced by @c expression must appear here.
    std::vector<std::string> wordable;
};

/// @brief Normalized in-memory schema consumed by ExpressionRuntime.
///
/// JSON loading converts the supported external schema into this structure
/// before constants, tables, and expressions are compiled.
struct ExpressionRuntimeConfig
{
    /// Named constants after loader normalization.
    ///
    /// JSON constant expressions are resolved to @c double before they are
    /// stored here. Unresolved or cyclic dependencies fail during loading.
    std::unordered_map<std::string, double> constants;

    /// Table function definitions available to compiled expressions.
    std::vector<TableConfig> tables;

    /// Runtime expressions available by name after loading.
    std::vector<ExpressionConfig> expressions;
};

} // namespace utility
} // namespace tfp

#endif
