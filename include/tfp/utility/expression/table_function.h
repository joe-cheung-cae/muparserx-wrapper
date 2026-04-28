#ifndef TFP_UTILITY_EXPRESSION_TABLE_FUNCTION_H
#define TFP_UTILITY_EXPRESSION_TABLE_FUNCTION_H

#include <array>
#include <string>
#include <vector>

namespace tfp
{
namespace utility
{

enum class ExtrapolationMode
{
    Clamp,
    Error,
    Linear
};

class TableFunction
{
public:
    // TableFunction is immutable after construction and may be shared across threads.
    TableFunction(std::string name, std::vector<std::array<double, 2> > data, ExtrapolationMode extrapolation);

    const std::string& Name() const noexcept;
    ExtrapolationMode Extrapolation() const noexcept;
    const std::vector<std::array<double, 2> >& Data() const noexcept;

    double Evaluate(double x) const;

private:
    std::string name_;
    std::vector<std::array<double, 2> > data_;
    ExtrapolationMode extrapolation_;
};

} // namespace utility
} // namespace tfp

#endif
