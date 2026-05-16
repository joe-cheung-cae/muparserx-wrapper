#ifndef TFP_UTILITY_EXPRESSION_TABLE_FUNCTION_H
#define TFP_UTILITY_EXPRESSION_TABLE_FUNCTION_H

#include <array>
#include <string>
#include <vector>

namespace tfp
{
namespace utility
{

/// @brief Controls how TableFunction handles inputs outside its configured
/// x-domain.
enum class ExtrapolationMode
{
    /// Return the nearest endpoint y value outside the table range.
    Clamp,
    /// Throw ExpressionError when x is outside the table range.
    Error,
    /// Extend the first or last segment linearly outside the table range.
    Linear
};

/// @brief Immutable one-dimensional lookup table with linear interpolation.
///
/// Construction validates and normalizes rows, so Data() exposes sorted
/// storage after a TableFunction has been created successfully.
class TableFunction
{
public:
    /// @brief Constructs an immutable table function.
    /// @param name Symbol name used when registering the table in a runtime.
    /// @param data Input rows as [x, y] pairs. Rows are sorted by x.
    /// @param extrapolation Out-of-range evaluation policy.
    ///
    /// The constructor rejects duplicate x coordinates and non-finite values.
    TableFunction(std::string name, std::vector<std::array<double, 2> > data, ExtrapolationMode extrapolation);

    /// @brief Returns the table symbol name.
    const std::string& Name() const noexcept;

    /// @brief Returns the configured out-of-range policy.
    ExtrapolationMode Extrapolation() const noexcept;

    /// @brief Returns the normalized [x, y] rows in sorted x order.
    const std::vector<std::array<double, 2> >& Data() const noexcept;

    /// @brief Evaluates the table at a single x coordinate.
    /// @param x Query coordinate.
    /// @return Interpolated or extrapolated y value.
    ///
    /// Uses linear interpolation between surrounding rows. Throws
    /// ExpressionError for non-finite x or for out-of-range x when the policy
    /// is ExtrapolationMode::Error.
    double Evaluate(double x) const;

private:
    std::string name_;
    std::vector<std::array<double, 2> > data_;
    ExtrapolationMode extrapolation_;
};

} // namespace utility
} // namespace tfp

#endif
