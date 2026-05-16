#ifndef TFP_UTILITY_EXPRESSION_RUNTIME_EXPRESSION_H
#define TFP_UTILITY_EXPRESSION_RUNTIME_EXPRESSION_H

#include "muparserx_backend.h"
#include "tfp/utility/expression/expression_config.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace tfp
{
namespace utility
{
namespace detail
{

// Implementation detail representing one compiled expression and its bound
// parser backend. Evaluation mutates backend variable storage, so callers must
// provide their own synchronization if sharing an instance across threads.
class RuntimeExpression
{
public:
    // Registers constants, table callbacks, and declared runtime variables,
    // then compiles config.expression. Throws ExpressionError if parser setup
    // fails or the expression references undeclared runtime variables.
    RuntimeExpression(const ExpressionConfig& config,
                      const std::unordered_map<std::string, double>& constants,
                      const std::unordered_map<std::string, std::shared_ptr<const TableFunction> >& tables);

    // Returns the expression lookup name. The reference is tied to this object.
    const std::string& Name() const noexcept;
    // Returns the ordered runtime-variable names used for positional argument
    // binding. The reference is tied to this object.
    const std::vector<std::string>& Wordable() const noexcept;
    // Returns Wordable().size() without inspecting the parser.
    std::size_t Arity() const noexcept;

    // Sets every runtime variable from args in wordable order and evaluates the
    // parser as double. Throws ExpressionError for arity mismatch or evaluation
    // failure.
    double Evaluate(const std::vector<double>& args);
    // Optimized unary evaluation path. Requires exactly one wordable variable
    // and throws ExpressionError if the expression is not unary or evaluation
    // fails.
    double EvaluateUnary(double x);
    // Translates variables by name into wordable order before evaluating.
    // Missing required variables and extra entries throw ExpressionError.
    double EvaluateMap(const std::unordered_map<std::string, double>& variables);

private:
    std::string name_;
    std::string expression_;
    std::vector<std::string> wordable_;
    std::unordered_map<std::string, std::size_t> variable_indices_;
    MuParserXBackend backend_;
};

} // namespace detail
} // namespace utility
} // namespace tfp

#endif
