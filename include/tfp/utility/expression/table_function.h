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
    Clamp,
    Error,
    Linear
};

class TableFunction
{
public:
    // TableFunction is immutable after construction and may be shared across
    // threads. The constructor sorts rows by x and rejects duplicate or
    // non-finite values before the table can be evaluated.
    TableFunction(std::string name, std::vector<std::array<double, 2> > data, ExtrapolationMode extrapolation);

    const std::string& Name() const noexcept;
    ExtrapolationMode Extrapolation() const noexcept;
    const std::vector<std::array<double, 2> >& Data() const noexcept;

    // Evaluates the table at x using linear interpolation between the two
    // surrounding rows and the configured extrapolation policy at the bounds.
    double Evaluate(double x) const;

private:
    std::string name_;
    std::vector<std::array<double, 2> > data_;
    ExtrapolationMode extrapolation_;
};

} // namespace utility
} // namespace tfp

#endif
