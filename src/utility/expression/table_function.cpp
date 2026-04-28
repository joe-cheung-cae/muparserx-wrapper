#include "tfp/utility/expression/table_function.h"

#include "tfp/utility/expression/expression_error.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace tfp
{
namespace utility
{
namespace
{

double Interpolate(const std::array<double, 2>& left, const std::array<double, 2>& right, double x)
{
    const double dx = right[0] - left[0];
    const double ratio = (x - left[0]) / dx;
    return left[1] + ratio * (right[1] - left[1]);
}

std::string TableMessage(const std::string& table_name, const std::string& message)
{
    return "table '" + table_name + "': " + message;
}

} // namespace

TableFunction::TableFunction(std::string name, std::vector<std::array<double, 2> > data, ExtrapolationMode extrapolation)
    : name_(std::move(name)),
      data_(std::move(data)),
      extrapolation_(extrapolation)
{
    if (data_.size() < 2)
    {
        throw ExpressionError(ExpressionErrorCode::TableError, TableMessage(name_, "requires at least two data rows"));
    }

    for (const auto& row : data_)
    {
        if (!std::isfinite(row[0]) || !std::isfinite(row[1]))
        {
            throw ExpressionError(ExpressionErrorCode::TableError, TableMessage(name_, "data contains non-finite value"));
        }
    }

    std::sort(data_.begin(), data_.end(), [](const std::array<double, 2>& a, const std::array<double, 2>& b) {
        return a[0] < b[0];
    });

    for (std::size_t i = 1; i < data_.size(); ++i)
    {
        if (data_[i - 1][0] == data_[i][0])
        {
            throw ExpressionError(ExpressionErrorCode::TableError, TableMessage(name_, "duplicate x value"));
        }
    }
}

const std::string& TableFunction::Name() const noexcept
{
    return name_;
}

ExtrapolationMode TableFunction::Extrapolation() const noexcept
{
    return extrapolation_;
}

const std::vector<std::array<double, 2> >& TableFunction::Data() const noexcept
{
    return data_;
}

double TableFunction::Evaluate(double x) const
{
    if (!std::isfinite(x))
    {
        throw ExpressionError(ExpressionErrorCode::EvaluationError, TableMessage(name_, "input is not finite"));
    }

    const auto& first = data_.front();
    const auto& second = data_[1];
    const auto& before_last = data_[data_.size() - 2];
    const auto& last = data_.back();

    if (x < first[0])
    {
        if (extrapolation_ == ExtrapolationMode::Clamp)
        {
            return first[1];
        }
        if (extrapolation_ == ExtrapolationMode::Linear)
        {
            return Interpolate(first, second, x);
        }
        throw ExpressionError(ExpressionErrorCode::EvaluationError, TableMessage(name_, "input below table range"));
    }

    if (x > last[0])
    {
        if (extrapolation_ == ExtrapolationMode::Clamp)
        {
            return last[1];
        }
        if (extrapolation_ == ExtrapolationMode::Linear)
        {
            return Interpolate(before_last, last, x);
        }
        throw ExpressionError(ExpressionErrorCode::EvaluationError, TableMessage(name_, "input above table range"));
    }

    auto upper = std::lower_bound(data_.begin(), data_.end(), x, [](const std::array<double, 2>& row, double value) {
        return row[0] < value;
    });

    if (upper != data_.end() && (*upper)[0] == x)
    {
        return (*upper)[1];
    }

    const auto& right = *upper;
    const auto& left = *(upper - 1);
    return Interpolate(left, right, x);
}

} // namespace utility
} // namespace tfp
