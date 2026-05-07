#include <tfp/utility/expression/expression_single_header.hpp>

void TouchSingleHeaderSymbol();

namespace
{

tfp::utility::ExpressionErrorCode KeepErrorCodeAlive()
{
    return tfp::utility::ExpressionErrorCode::ConfigError;
}

} // namespace

void TouchSecondaryTranslationUnit()
{
    (void)KeepErrorCodeAlive();
    TouchSingleHeaderSymbol();
}
