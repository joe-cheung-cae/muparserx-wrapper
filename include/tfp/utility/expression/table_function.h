#ifndef TFP_UTILITY_EXPRESSION_TABLE_FUNCTION_H
#define TFP_UTILITY_EXPRESSION_TABLE_FUNCTION_H

#include <array>
#include <string>
#include <vector>

namespace tfp
{
namespace utility
{

// ExtrapolationMode controls how TableFunction handles inputs outside the
// minimum and maximum configured x coordinates.
enum class ExtrapolationMode
{
    // Return the nearest endpoint y value outside the table range.
    Clamp,
    // Throw ExpressionError when x is outside the table range.
    Error,
    // Extend the first or last segment linearly outside the table range.
    Linear
};

// Immutable one-dimensional lookup table with linear interpolation between
// rows. Construction validates and normalizes rows, so Data() exposes sorted
// storage after a TableFunction has been created successfully.
class TableFunction
{
public:
    // TableFunction is immutable after construction and may be shared across
    // threads. The constructor sorts rows by x and rejects duplicate or
    // non-finite values before the table can be evaluated.
    TableFunction(std::string name, std::vector<std::array<double, 2> > data, ExtrapolationMode extrapolation);

    // Returns the table symbol name. The reference remains valid for the
    // lifetime of the TableFunction.
    const std::string& Name() const noexcept;
    // Returns the configured out-of-range policy.
    ExtrapolationMode Extrapolation() const noexcept;
    // Returns sorted [x, y] rows. The reference remains valid for the lifetime
    // of the TableFunction and must not be used after destruction.
    const std::vector<std::array<double, 2> >& Data() const noexcept;

    // Evaluates the table at x using linear interpolation between the two
    // surrounding rows and the configured extrapolation policy at the bounds.
    // Throws ExpressionError for non-finite x or out-of-range x when the policy
    // is ExtrapolationMode::Error.
    double Evaluate(double x) const;

private:
    std::string name_;
    std::vector<std::array<double, 2> > data_;
    ExtrapolationMode extrapolation_;
};

} // namespace utility
} // namespace tfp

#endif
