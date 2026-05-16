#ifndef TFP_UTILITY_EXPRESSION_SINGLE_HEADER_HPP
#define TFP_UTILITY_EXPRESSION_SINGLE_HEADER_HPP

// Header-only snapshot of the current expression runtime API.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <mpError.h>
#include <mpICallback.h>
#include <mpIToken.h>
#include <mpParser.h>
#include <mpValue.h>
#include <mpVariable.h>

namespace tfp
{
namespace utility
{

// ExpressionErrorCode identifies which phase of the runtime pipeline reported
// the failure so callers can distinguish configuration problems from runtime
// lookup, compile, or evaluation failures.
enum class ExpressionErrorCode
{
    // JSON or in-memory configuration schema validation failed.
    ConfigError,
    // Table configuration failed validation during construction.
    TableError,
    // Constant dependency resolution or constant expression evaluation failed.
    ConstantError,
    // Parser binding, symbol definition, or expression compilation failed.
    CompileError,
    // Evaluation failed after an expression or table had been compiled.
    EvaluationError,
    // A requested expression or symbol name was not present.
    NotFound,
    // A caller supplied the wrong handle state, arity, variable set, or index.
    InvalidArgument
};

// Exception type thrown by the expression runtime. The inherited what() message
// is human-readable, while Code() provides stable phase-level classification.
class ExpressionError : public std::runtime_error
{
public:
    // Stores the supplied classification and message. The message is also
    // passed to std::runtime_error and remains available through what().
    ExpressionError(ExpressionErrorCode code, const std::string& message);

    // Returns the classification associated with this exception without
    // allocating or throwing.
    ExpressionErrorCode Code() const noexcept;

private:
    ExpressionErrorCode code_;
};

} // namespace utility
} // namespace tfp

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

namespace tfp
{
namespace utility
{

// TableConfig describes one named one-dimensional lookup table loaded into the
// runtime before expressions are compiled.
struct TableConfig
{
    // Symbol name used when expressions call the table as a function.
    std::string name;
    // Rows are [x, y] pairs. Construction validates finite values, sorts rows
    // by x, and rejects duplicate x coordinates.
    std::vector<std::array<double, 2> > data;
    // Boundary policy used when evaluating outside the configured x range.
    ExtrapolationMode extrapolation = ExtrapolationMode::Clamp;
};

// ExpressionConfig describes one compiled expression entry. wordable defines
// the ordered runtime-variable list and therefore the positional argument order
// used by ExpressionHandle::Evaluate().
struct ExpressionConfig
{
    // Unique expression symbol name used for lookup from ExpressionRuntime.
    std::string name;
    // Parser expression text. It may refer to constants, tables, and the
    // runtime variables listed in wordable.
    std::string expression;
    // Ordered runtime-variable list. Names must be unique within the expression
    // and every runtime variable referenced by expression must appear here.
    std::vector<std::string> wordable;
};

// ExpressionRuntimeConfig is the normalized in-memory schema produced by the
// loader before constants, tables, and expressions are compiled.
struct ExpressionRuntimeConfig
{
    // Named constants after loader normalization. JSON constant expressions are
    // resolved to double before they are stored here; unresolved/cyclic
    // dependencies fail during loading.
    std::unordered_map<std::string, double> constants;
    // Table function definitions available to compiled expressions.
    std::vector<TableConfig> tables;
    // Runtime expressions available by name after loading.
    std::vector<ExpressionConfig> expressions;
};

} // namespace utility
} // namespace tfp

namespace tfp
{
namespace utility
{
namespace detail
{
// Internal compiled expression representation shared by public handles.
class RuntimeExpression;
// Internal pimpl type that creates handles for ExpressionRuntime.
class ExpressionRuntimeImpl;
}

// ExpressionHandle is a lightweight reference to a compiled expression that
// evaluates arguments in the order declared by ExpressionConfig::wordable.
class ExpressionHandle
{
public:
    // A default-constructed handle is empty and throws ExpressionError when it
    // is evaluated or queried for arity.
    ExpressionHandle();

    // Evaluates the compiled expression with positional arguments matching the
    // configured wordable order exactly. Throws ExpressionError if the handle
    // is empty, the argument count is wrong, or parser evaluation fails.
    double Evaluate(const std::vector<double>& args) const;
    // Returns the number of positional arguments required by Evaluate(). Throws
    // ExpressionError if the handle is empty.
    std::size_t Arity() const;

private:
    friend class ExpressionRuntime;
    friend class detail::ExpressionRuntimeImpl;
    // Binds the handle to compiled shared state. Runtime factory methods use
    // this constructor so handles can outlive moved or reloaded runtime objects
    // as snapshots of the expression state at lookup time.
    explicit ExpressionHandle(std::shared_ptr<detail::RuntimeExpression> expression);

    std::shared_ptr<detail::RuntimeExpression> expression_;
};

// UnaryExpressionHandle is the specialized fast path for expressions with one
// runtime variable, avoiding vector construction at each evaluation.
class UnaryExpressionHandle
{
public:
    // A default-constructed unary handle is empty and throws ExpressionError
    // if Evaluate() is called before it is bound to a compiled expression.
    UnaryExpressionHandle();

    // Evaluates the compiled unary expression with a single runtime variable.
    // Throws ExpressionError if the handle is empty or evaluation fails.
    double Evaluate(double x) const;

private:
    friend class ExpressionRuntime;
    friend class detail::ExpressionRuntimeImpl;
    // Binds the handle to a compiled expression already checked for arity 1 by
    // ExpressionRuntime::GetUnaryExpression(). The handle is a snapshot of the
    // expression state at lookup time.
    explicit UnaryExpressionHandle(std::shared_ptr<detail::RuntimeExpression> expression);

    std::shared_ptr<detail::RuntimeExpression> expression_;
};

} // namespace utility
} // namespace tfp

namespace tfp
{
namespace utility
{
namespace detail
{
// Private implementation that owns compiled expressions and parser state.
class ExpressionRuntimeImpl;
}

// ExpressionRuntime owns the compiled expression graph created from a JSON
// configuration and serves as the entry point for loading and evaluating it.
class ExpressionRuntime
{
public:
    // Evaluate mutates parser-bound variable storage. Use one runtime instance
    // per thread when evaluating expressions concurrently.
    ExpressionRuntime();
    // Releases compiled expressions and parser resources owned by this runtime.
    ~ExpressionRuntime();

    // Transfers ownership of compiled runtime state. Existing handles remain
    // valid because they share compiled expressions independently.
    ExpressionRuntime(ExpressionRuntime&&) noexcept;
    // Replaces this runtime with another runtime's compiled state.
    ExpressionRuntime& operator=(ExpressionRuntime&&) noexcept;

    // Runtime state is unique because it owns mutable parser bindings.
    ExpressionRuntime(const ExpressionRuntime&) = delete;
    // Copy assignment is disabled for the same ownership and thread-safety
    // reasons as copy construction.
    ExpressionRuntime& operator=(const ExpressionRuntime&) = delete;

    // Constructs a runtime and loads an in-memory config before returning it.
    // Throws ExpressionError for the same failures as LoadFromConfig().
    [[nodiscard]] static ExpressionRuntime CreateFromConfig(const ExpressionRuntimeConfig& config);
    // Constructs a runtime from JSON text. Throws ExpressionError when parsing,
    // validation, or compilation fails.
    [[nodiscard]] static ExpressionRuntime CreateFromJsonString(const std::string& json_text);
    // Constructs a runtime from a parsed JSON object. Throws ExpressionError
    // when schema validation or compilation fails.
    [[nodiscard]] static ExpressionRuntime CreateFromJsonObject(const nlohmann::json& json_object);
    // Constructs a runtime from a JSON file path. Throws ExpressionError when
    // file loading, parsing, validation, or compilation fails.
    [[nodiscard]] static ExpressionRuntime CreateFromJsonFile(const std::string& path);

    // Loads constants, tables, and expressions from an in-memory normalized
    // configuration object and replaces any previously compiled expressions
    // owned by this runtime.
    void LoadFromConfig(const ExpressionRuntimeConfig& config);
    // Loads constants, tables, and expressions from a JSON document string and
    // replaces any previously compiled expressions owned by this runtime.
    void LoadFromJsonString(const std::string& json_text);
    // Loads constants, tables, and expressions from a parsed JSON object and
    // replaces any previously compiled expressions owned by this runtime.
    // Schema validation and JSON conversion failures are reported as
    // ExpressionError.
    void LoadFromJsonObject(const nlohmann::json& json_object);
    // Loads the same JSON model from disk before compiling it into runtime
    // state. File and JSON validation failures are reported as ExpressionError.
    void LoadFromJsonFile(const std::string& path);

    // Returns a snapshot handle for ordered expression argument evaluation. The
    // handle shares compiled expression state and remains valid after this
    // runtime is moved or reloaded; after a reload it continues to evaluate the
    // old compiled expression. Concurrent evaluation still follows the runtime
    // thread-safety rules because handles use mutable parser-bound state.
    ExpressionHandle GetExpression(const std::string& name) const;
    // Returns a snapshot handle specialized for expressions that declare exactly
    // one runtime variable in wordable order. This is expression-only; use
    // EvaluateUnary() for unified constant/table/expression evaluation.
    UnaryExpressionHandle GetUnaryExpression(const std::string& name) const;

    // Returns the ordered runtime argument names for a named item loaded into
    // this runtime. Constants return an empty list, tables return {"x"}, and
    // expressions return their declared wordable order. Throws ExpressionError
    // if the name does not exist.
    std::vector<std::string> GetArgumentNames(const std::string& name) const;

    // Evaluates a loaded runtime item by name. Expressions require the supplied
    // variable map to match wordable exactly, tables use the single supplied
    // variable value (conventionally "x"), and constants require an empty map.
    double Evaluate(const std::string& name, const std::unordered_map<std::string, double>& variables) const;

    // Evaluates a loaded one-dimensional runtime item by name. Constants ignore
    // x, tables evaluate at x, and expressions must declare exactly one runtime
    // variable.
    double EvaluateUnary(const std::string& name, double x) const;

private:
    std::unique_ptr<detail::ExpressionRuntimeImpl> impl_;
};

} // namespace utility
} // namespace tfp

namespace tfp
{
namespace utility
{
namespace detail
{

// Thin implementation detail around muparserx that owns parser-bound variable
// storage and converts muparserx errors into ExpressionError. Methods are not
// synchronized; each RuntimeExpression owns the backend it mutates.
class MuParserXBackend
{
public:
    // Creates an empty parser backend with no constants, variables, tables, or
    // expression text registered.
    MuParserXBackend();

    // Defines or replaces a numeric constant in the parser symbol table.
    void DefineConstant(const std::string& name, double value);
    // Creates mutable parser variables in the supplied order. The backing
    // storage remains owned by this backend for the backend lifetime.
    void DefineVariables(const std::vector<std::string>& names);
    // Updates one parser-bound variable by positional index.
    void SetVariable(std::size_t index, double value);
    // Defines or replaces a table function callback. The shared_ptr keeps the
    // immutable TableFunction alive for callback use.
    void DefineTable(const std::shared_ptr<const TableFunction>& table);
    // Sets parser expression text and performs muparserx parsing/validation.
    void SetExpression(const std::string& expression);

    // Returns variable symbols referenced by the current expression. Constants
    // and table callback names are excluded by muparserx.
    std::vector<std::string> GetReferencedVariables() const;
    // Evaluates the current expression and converts the parser value to double.
    double EvalAsDouble() const;

private:
    mup::ParserX parser_;
    std::vector<mup::Value> variable_storage_;
};

// Evaluates a constant expression with only already-resolved constants in
// scope. Unknown symbols, parser errors, and evaluation errors are wrapped as
// ExpressionError so the resolver can classify constant failures consistently.
double EvaluateConstantExpression(const std::string& constant_name,
                                  const std::string& expression,
                                  const std::unordered_map<std::string, double>& constants);

} // namespace detail
} // namespace utility
} // namespace tfp

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

namespace tfp
{
namespace utility
{
namespace detail
{

// Implementation detail used by ExpressionRuntimeImpl to compile one expression
// config against already-resolved constants and immutable table functions.
// Returns shared compiled state so public handles can extend expression
// lifetime beyond runtime moves. Throws ExpressionError on parser binding or
// validation failures.
std::shared_ptr<RuntimeExpression> CompileRuntimeExpression(
    const ExpressionConfig& config,
    const std::unordered_map<std::string, double>& constants,
    const std::unordered_map<std::string, std::shared_ptr<const TableFunction> >& tables);

} // namespace detail
} // namespace utility
} // namespace tfp

namespace tfp
{
namespace utility
{

// Resolves raw named constant expressions into double values before runtime
// expressions are compiled. Constants may refer to constants already resolved
// in dependency order, but unknown symbols and cycles are reported as
// ExpressionError with ConstantError.
std::unordered_map<std::string, double> ResolveConstants(
    const std::unordered_map<std::string, std::string>& raw_constants);

} // namespace utility
} // namespace tfp

namespace tfp
{
namespace utility
{

// Parses and validates the supported JSON schema into the normalized in-memory
// configuration used by the compiler. Accepts both current top-level sections
// and the legacy functions array, and throws ExpressionError with ConfigError
// for malformed JSON or schema violations.
ExpressionRuntimeConfig LoadExpressionRuntimeConfigFromJsonString(const std::string& json_text);
// Validates and normalizes an already parsed JSON object. Non-object roots,
// schema failures, and JSON conversion failures are reported as ExpressionError
// with ConfigError.
ExpressionRuntimeConfig LoadExpressionRuntimeConfigFromJsonObject(const nlohmann::json& json_object);
// Reads a JSON document from path and delegates to
// LoadExpressionRuntimeConfigFromJsonString(). File-open, parse, and schema
// failures are reported as ExpressionError with ConfigError.
ExpressionRuntimeConfig LoadExpressionRuntimeConfigFromJsonFile(const std::string& path);

} // namespace utility
} // namespace tfp

namespace tfp
{
namespace utility
{

inline ExpressionError::ExpressionError(ExpressionErrorCode code, const std::string& message)
    : std::runtime_error(message),
      code_(code)
{
}

inline ExpressionErrorCode ExpressionError::Code() const noexcept
{
    return code_;
}

} // namespace utility
} // namespace tfp


namespace tfp
{
namespace utility
{
namespace
{

inline double Interpolate(const std::array<double, 2>& left, const std::array<double, 2>& right, double x)
{
    const double dx = right[0] - left[0];
    const double ratio = (x - left[0]) / dx;
    return left[1] + ratio * (right[1] - left[1]);
}

inline std::string TableMessage(const std::string& table_name, const std::string& message)
{
    return "table '" + table_name + "': " + message;
}

} // namespace

inline TableFunction::TableFunction(std::string name, std::vector<std::array<double, 2> > data, ExtrapolationMode extrapolation)
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

inline const std::string& TableFunction::Name() const noexcept
{
    return name_;
}

inline ExtrapolationMode TableFunction::Extrapolation() const noexcept
{
    return extrapolation_;
}

inline const std::vector<std::array<double, 2> >& TableFunction::Data() const noexcept
{
    return data_;
}

inline double TableFunction::Evaluate(double x) const
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


namespace tfp
{
namespace utility
{
namespace detail
{

class MuParserXTableCallback : public mup::ICallback
{
public:
    explicit MuParserXTableCallback(std::shared_ptr<const TableFunction> table)
        : mup::ICallback(mup::cmFUNC, table->Name().c_str(), 1),
          table_(std::move(table))
    {
    }

    void Eval(mup::ptr_val_type& ret, const mup::ptr_val_type* arg, int argc) override
    {
        if (argc != 1)
        {
            throw ExpressionError(ExpressionErrorCode::EvaluationError, "table function expects exactly one argument");
        }
        *ret = table_->Evaluate(arg[0]->GetFloat());
    }

    const mup::char_type* GetDesc() const override
    {
        return "";
    }

    mup::IToken* Clone() const override
    {
        return new MuParserXTableCallback(*this);
    }

private:
    std::shared_ptr<const TableFunction> table_;
};

inline mup::ICallback* CreateTableCallback(const std::shared_ptr<const TableFunction>& table)
{
    return new MuParserXTableCallback(table);
}

} // namespace detail
} // namespace utility
} // namespace tfp


namespace tfp
{
namespace utility
{
namespace detail
{

inline mup::ICallback* CreateTableCallback(const std::shared_ptr<const TableFunction>& table);

namespace
{

inline std::string ToMessage(const mup::ParserError& error)
{
    return error.GetMsg();
}

} // namespace

inline MuParserXBackend::MuParserXBackend()
    : parser_()
{
}

inline void MuParserXBackend::DefineConstant(const std::string& name, double value)
{
    try
    {
        if (parser_.IsConstDefined(name))
        {
            parser_.RemoveConst(name);
        }
        parser_.DefineConst(name, mup::Value(value));
    }
    catch (const mup::ParserError& error)
    {
        throw ExpressionError(ExpressionErrorCode::CompileError,
                              "failed to define constant '" + name + "': " + ToMessage(error));
    }
}

inline void MuParserXBackend::DefineVariables(const std::vector<std::string>& names)
{
    // muparserx variables are bound to mutable Value objects, so the storage
    // vector must stay alive for the full lifetime of the compiled expression.
    variable_storage_.assign(names.size(), mup::Value(0.0));
    for (std::size_t i = 0; i < names.size(); ++i)
    {
        try
        {
            parser_.DefineVar(names[i], mup::Variable(&variable_storage_[i]));
        }
        catch (const mup::ParserError& error)
        {
            throw ExpressionError(ExpressionErrorCode::CompileError,
                                  "failed to define variable '" + names[i] + "': " + ToMessage(error));
        }
    }
}

inline void MuParserXBackend::SetVariable(std::size_t index, double value)
{
    if (index >= variable_storage_.size())
    {
        throw ExpressionError(ExpressionErrorCode::InvalidArgument, "variable index out of range");
    }
    variable_storage_[index] = value;
}

inline void MuParserXBackend::DefineTable(const std::shared_ptr<const TableFunction>& table)
{
    try
    {
        if (parser_.IsFunDefined(table->Name()))
        {
            parser_.RemoveFun(table->Name());
        }
        parser_.DefineFun(mup::ptr_cal_type(CreateTableCallback(table)));
    }
    catch (const mup::ParserError& error)
    {
        throw ExpressionError(ExpressionErrorCode::CompileError,
                              "failed to define table function '" + table->Name() + "': " + ToMessage(error));
    }
}

inline void MuParserXBackend::SetExpression(const std::string& expression)
{
    try
    {
        parser_.SetExpr(expression);
    }
    catch (const mup::ParserError& error)
    {
        throw ExpressionError(ExpressionErrorCode::CompileError, "failed to set expression: " + ToMessage(error));
    }
}

inline std::vector<std::string> MuParserXBackend::GetReferencedVariables() const
{
    try
    {
        std::vector<std::string> variables;
        // muparserx reports only variable symbols here, so constants and table
        // callbacks can already be registered without appearing in this list.
        const mup::var_maptype& referenced = parser_.GetExprVar();
        for (mup::var_maptype::const_iterator it = referenced.begin(); it != referenced.end(); ++it)
        {
            variables.push_back(it->first);
        }
        return variables;
    }
    catch (const mup::ParserError& error)
    {
        throw ExpressionError(ExpressionErrorCode::CompileError,
                              "failed to inspect expression variables: " + ToMessage(error));
    }
}

inline double MuParserXBackend::EvalAsDouble() const
{
    try
    {
        return parser_.Eval().GetFloat();
    }
    catch (const ExpressionError&)
    {
        throw;
    }
    catch (const mup::ParserError& error)
    {
        throw ExpressionError(ExpressionErrorCode::EvaluationError, "expression evaluation failed: " + ToMessage(error));
    }
    catch (const std::exception& error)
    {
        throw ExpressionError(ExpressionErrorCode::EvaluationError, std::string("expression evaluation failed: ") + error.what());
    }
}

inline double EvaluateConstantExpression(const std::string& constant_name,
                                  const std::string& expression,
                                  const std::unordered_map<std::string, double>& constants)
{
    // Constant evaluation uses a fresh backend with only already-resolved
    // constants so runtime variables and table functions are never in scope.
    MuParserXBackend backend;
    for (std::unordered_map<std::string, double>::const_iterator it = constants.begin(); it != constants.end(); ++it)
    {
        backend.DefineConstant(it->first, it->second);
    }
    backend.SetExpression(expression);

    const std::vector<std::string> referenced_variables = backend.GetReferencedVariables();
    if (!referenced_variables.empty())
    {
        throw ExpressionError(ExpressionErrorCode::ConstantError,
                              "constant '" + constant_name + "' references unknown symbol '" + referenced_variables.front() + "'");
    }

    try
    {
        return backend.EvalAsDouble();
    }
    catch (const ExpressionError& error)
    {
        throw ExpressionError(ExpressionErrorCode::ConstantError,
                              "constant '" + constant_name + "' failed to evaluate: " + error.what());
    }
}

} // namespace detail
} // namespace utility
} // namespace tfp


namespace tfp
{
namespace utility
{
namespace detail
{

inline RuntimeExpression::RuntimeExpression(
    const ExpressionConfig& config,
    const std::unordered_map<std::string, double>& constants,
    const std::unordered_map<std::string, std::shared_ptr<const TableFunction> >& tables)
    : name_(config.name),
      expression_(config.expression),
      wordable_(config.wordable)
{
    // Constants and tables are registered before runtime variables so the
    // parser sees the same symbol set during validation and later evaluation.
    for (std::unordered_map<std::string, double>::const_iterator it = constants.begin(); it != constants.end(); ++it)
    {
        backend_.DefineConstant(it->first, it->second);
    }

    for (std::unordered_map<std::string, std::shared_ptr<const TableFunction> >::const_iterator it = tables.begin();
         it != tables.end(); ++it)
    {
        backend_.DefineTable(it->second);
    }

    // wordable defines the ordered runtime-variable contract exposed through
    // ExpressionHandle::Evaluate(std::vector<double>) and EvaluateMap().
    for (std::size_t i = 0; i < wordable_.size(); ++i)
    {
        variable_indices_[wordable_[i]] = i;
    }
    backend_.DefineVariables(wordable_);

    try
    {
        backend_.SetExpression(expression_);
    }
    catch (const ExpressionError& error)
    {
        throw ExpressionError(ExpressionErrorCode::CompileError,
                              "expression '" + name_ + "' compile failed: " + error.what());
    }

    std::vector<std::string> referenced_variables;
    try
    {
        referenced_variables = backend_.GetReferencedVariables();
    }
    catch (const ExpressionError& error)
    {
        throw ExpressionError(ExpressionErrorCode::CompileError,
                              "expression '" + name_ + "' compile failed: " + error.what());
    }
    for (std::vector<std::string>::const_iterator it = referenced_variables.begin(); it != referenced_variables.end(); ++it)
    {
        if (variable_indices_.find(*it) == variable_indices_.end())
        {
            throw ExpressionError(ExpressionErrorCode::CompileError,
                                  "expression '" + name_ + "' references undeclared runtime variable '" + *it + "'");
        }
    }
}

inline const std::string& RuntimeExpression::Name() const noexcept
{
    return name_;
}

inline const std::vector<std::string>& RuntimeExpression::Wordable() const noexcept
{
    return wordable_;
}

inline std::size_t RuntimeExpression::Arity() const noexcept
{
    return wordable_.size();
}

inline double RuntimeExpression::Evaluate(const std::vector<double>& args)
{
    if (args.size() != wordable_.size())
    {
        throw ExpressionError(ExpressionErrorCode::InvalidArgument,
                              "expression '" + name_ + "' expected " + std::to_string(wordable_.size()) +
                                  " arguments, got " + std::to_string(args.size()));
    }

    for (std::size_t i = 0; i < args.size(); ++i)
    {
        backend_.SetVariable(i, args[i]);
    }

    try
    {
        return backend_.EvalAsDouble();
    }
    catch (const ExpressionError& error)
    {
        throw ExpressionError(ExpressionErrorCode::EvaluationError,
                              "expression '" + name_ + "' evaluation failed: " + error.what());
    }
}

inline double RuntimeExpression::EvaluateUnary(double x)
{
    if (wordable_.size() != 1)
    {
        throw ExpressionError(ExpressionErrorCode::InvalidArgument,
                              "expression '" + name_ + "' expected 1 argument, got " + std::to_string(wordable_.size()));
    }

    backend_.SetVariable(0, x);
    try
    {
        return backend_.EvalAsDouble();
    }
    catch (const ExpressionError& error)
    {
        throw ExpressionError(ExpressionErrorCode::EvaluationError,
                              "expression '" + name_ + "' evaluation failed: " + error.what());
    }
}

inline double RuntimeExpression::EvaluateMap(const std::unordered_map<std::string, double>& variables)
{
    // The map-based API preserves the same evaluation semantics as positional
    // handles by translating names into the canonical wordable argument order.
    std::vector<double> args(wordable_.size());
    for (std::size_t i = 0; i < wordable_.size(); ++i)
    {
        std::unordered_map<std::string, double>::const_iterator value = variables.find(wordable_[i]);
        if (value == variables.end())
        {
            throw ExpressionError(ExpressionErrorCode::InvalidArgument,
                                  "expression '" + name_ + "' missing variable '" + wordable_[i] + "'");
        }
        args[i] = value->second;
    }

    if (variables.size() != wordable_.size())
    {
        for (std::unordered_map<std::string, double>::const_iterator it = variables.begin(); it != variables.end(); ++it)
        {
            if (variable_indices_.find(it->first) == variable_indices_.end())
            {
                throw ExpressionError(ExpressionErrorCode::InvalidArgument,
                                      "expression '" + name_ + "' received unexpected variable '" + it->first + "'");
            }
        }

        throw ExpressionError(ExpressionErrorCode::InvalidArgument,
                              "expression '" + name_ + "' expected " + std::to_string(wordable_.size()) +
                                  " variables, got " + std::to_string(variables.size()));
    }

    return Evaluate(args);
}

} // namespace detail
} // namespace utility
} // namespace tfp


namespace tfp
{
namespace utility
{
namespace detail
{

inline std::shared_ptr<RuntimeExpression> CompileRuntimeExpression(
    const ExpressionConfig& config,
    const std::unordered_map<std::string, double>& constants,
    const std::unordered_map<std::string, std::shared_ptr<const TableFunction> >& tables)
{
    return std::make_shared<RuntimeExpression>(config, constants, tables);
}

} // namespace detail
} // namespace utility
} // namespace tfp


namespace tfp
{
namespace utility
{

inline std::unordered_map<std::string, double> ResolveConstants(
    const std::unordered_map<std::string, std::string>& raw_constants)
{
    std::unordered_map<std::string, double> resolved;
    std::set<std::string> unresolved;
    for (std::unordered_map<std::string, std::string>::const_iterator it = raw_constants.begin(); it != raw_constants.end(); ++it)
    {
        unresolved.insert(it->first);
    }

    while (!unresolved.empty())
    {
        bool made_progress = false;
        std::vector<std::string> resolved_this_pass;

        for (std::set<std::string>::const_iterator it = unresolved.begin(); it != unresolved.end(); ++it)
        {
            const std::string& name = *it;
            try
            {
                // A constant can be resolved as soon as every referenced symbol
                // has already been resolved in an earlier pass.
                resolved[name] = detail::EvaluateConstantExpression(name, raw_constants.at(name), resolved);
                resolved_this_pass.push_back(name);
                made_progress = true;
            }
            catch (const ExpressionError&)
            {
                // Failures are deferred until the end of the pass so other
                // constants that no longer have unresolved dependencies can
                // still make progress.
            }
        }

        for (std::vector<std::string>::const_iterator it = resolved_this_pass.begin(); it != resolved_this_pass.end(); ++it)
        {
            unresolved.erase(*it);
        }

        if (!made_progress)
        {
            // If an entire pass cannot resolve anything, the remaining names
            // must contain either an unknown symbol reference or a dependency
            // cycle between constants.
            throw ExpressionError(ExpressionErrorCode::ConstantError,
                                  "constants contain an unknown symbol or circular dependency near '" + *unresolved.begin() + "'");
        }
    }

    return resolved;
}

} // namespace utility
} // namespace tfp


namespace tfp
{
namespace utility
{
namespace
{

using Json = nlohmann::json;

inline ExpressionError ConfigError(const std::string& message)
{
    return ExpressionError(ExpressionErrorCode::ConfigError, message);
}

inline void AddConstant(const std::string& name,
                        const std::string& value,
                        std::unordered_map<std::string, std::string>& raw_constants)
{
    if (name.empty())
    {
        throw ConfigError("name must not be empty");
    }
    if (raw_constants.find(name) != raw_constants.end())
    {
        throw ConfigError("duplicate global symbol '" + name + "'");
    }
    raw_constants[name] = value;
}

inline void RequireNonEmptyName(const std::string& context, const std::string& name)
{
    if (name.empty())
    {
        throw ConfigError(context + ": name must not be empty");
    }
}

inline ExtrapolationMode ParseExtrapolation(const std::string& table_name, const Json& table)
{
    // Tables default to clamp extrapolation so callers can omit the field when
    // boundary values should simply stick to the nearest endpoint.
    if (!table.contains("extrapolation"))
    {
        return ExtrapolationMode::Clamp;
    }

    if (!table["extrapolation"].is_string())
    {
        throw ConfigError("table '" + table_name + "': extrapolation must be a string");
    }

    const std::string mode = table["extrapolation"].get<std::string>();
    if (mode == "clamp")
    {
        return ExtrapolationMode::Clamp;
    }
    if (mode == "error")
    {
        return ExtrapolationMode::Error;
    }
    if (mode == "linear")
    {
        return ExtrapolationMode::Linear;
    }

    throw ConfigError("table '" + table_name + "': unsupported extrapolation '" + mode + "'");
}

inline std::vector<std::array<double, 2> > ParseTableData(const std::string& table_name, const Json& table)
{
    // The loader accepts raw JSON rows and normalizes them into the fixed
    // numeric table representation expected by TableFunction.
    if (!table.contains("data"))
    {
        throw ConfigError("table '" + table_name + "': missing data");
    }
    if (!table["data"].is_array())
    {
        throw ConfigError("table '" + table_name + "': data must be an array");
    }

    std::vector<std::array<double, 2> > rows;
    for (std::size_t i = 0; i < table["data"].size(); ++i)
    {
        const Json& row = table["data"][i];
        if (!row.is_array() || row.size() != 2)
        {
            throw ConfigError("table '" + table_name + "': data row " + std::to_string(i) + " must be [x, y]");
        }
        if (!row[0].is_number() || !row[1].is_number())
        {
            throw ConfigError("table '" + table_name + "': data row " + std::to_string(i) + " values must be numeric");
        }

        double x = 0.0;
        double y = 0.0;
        try
        {
            x = row[0].get<double>();
            y = row[1].get<double>();
        }
        catch (const std::exception& error)
        {
            throw ConfigError("table '" + table_name + "': data row " + std::to_string(i) +
                              " values cannot be represented as double: " + error.what());
        }

        if (!std::isfinite(x) || !std::isfinite(y))
        {
            throw ConfigError("table '" + table_name + "': data row " + std::to_string(i) +
                              " contains non-finite value");
        }

        rows.push_back(std::array<double, 2>{{x, y}});
    }
    return rows;
}

inline TableConfig ParseTableObject(const std::string& table_name, const Json& table_json)
{
    if (!table_json.is_object())
    {
        throw ConfigError("table '" + table_name + "' must be an object");
    }

    TableConfig table;
    table.name = table_name;
    table.extrapolation = ParseExtrapolation(table.name, table_json);
    table.data = ParseTableData(table.name, table_json);
    return table;
}

inline void ParseConstants(const Json& root, std::unordered_map<std::string, std::string>& raw_constants)
{
    if (!root.contains("constants"))
    {
        return;
    }
    if (!root["constants"].is_object())
    {
        throw ConfigError("constants must be an object");
    }

    for (Json::const_iterator it = root["constants"].begin(); it != root["constants"].end(); ++it)
    {
        if (!it.value().is_string())
        {
            throw ConfigError("constant '" + it.key() + "' must be a string expression");
        }
        AddConstant(it.key(), it.value().get<std::string>(), raw_constants);
    }
}

inline void ParseTables(const Json& root, ExpressionRuntimeConfig& config)
{
    if (!root.contains("tables"))
    {
        return;
    }

    if (root["tables"].is_array())
    {
        for (std::size_t i = 0; i < root["tables"].size(); ++i)
        {
            const Json& table_json = root["tables"][i];
            if (!table_json.is_object())
            {
                throw ConfigError("tables[" + std::to_string(i) + "] must be an object");
            }
            if (!table_json.contains("name") || !table_json["name"].is_string())
            {
                throw ConfigError("tables[" + std::to_string(i) + "]: name must be a string");
            }

            const std::string name = table_json["name"].get<std::string>();
            RequireNonEmptyName("tables[" + std::to_string(i) + "]", name);
            config.tables.push_back(ParseTableObject(name, table_json));
        }
        return;
    }

    if (!root["tables"].is_object())
    {
        throw ConfigError("tables must be an array or object");
    }

    for (Json::const_iterator it = root["tables"].begin(); it != root["tables"].end(); ++it)
    {
        RequireNonEmptyName("table '" + it.key() + "'", it.key());
        config.tables.push_back(ParseTableObject(it.key(), it.value()));
    }
}

inline ExpressionConfig ParseExpressionObject(const std::string& expression_name, const Json& expression_json)
{
    if (!expression_json.is_object())
    {
        throw ConfigError("expression '" + expression_name + "' must be an object");
    }
    if (!expression_json.contains("expression") || !expression_json["expression"].is_string())
    {
        throw ConfigError("expression '" + expression_name + "': expression must be a string");
    }
    if (!expression_json.contains("wordable") || !expression_json["wordable"].is_array())
    {
        throw ConfigError("expression '" + expression_name + "': wordable must be a string array");
    }

    ExpressionConfig expression;
    expression.name = expression_name;
    expression.expression = expression_json["expression"].get<std::string>();

    std::set<std::string> seen;
    for (std::size_t i = 0; i < expression_json["wordable"].size(); ++i)
    {
        const Json& variable = expression_json["wordable"][i];
        if (!variable.is_string())
        {
            throw ConfigError("expression '" + expression_name + "': wordable values must be strings");
        }
        const std::string variable_name = variable.get<std::string>();
        if (!seen.insert(variable_name).second)
        {
            throw ConfigError("expression '" + expression_name + "': duplicate wordable variable '" + variable_name + "'");
        }
        expression.wordable.push_back(variable_name);
    }

    return expression;
}

inline void ParseExpressions(const Json& root, ExpressionRuntimeConfig& config)
{
    if (!root.contains("expressions"))
    {
        return;
    }

    // The preferred representation is an array of named objects, but the
    // loader still accepts the older object form for compatibility.
    if (root["expressions"].is_array())
    {
        for (std::size_t i = 0; i < root["expressions"].size(); ++i)
        {
            const Json& expression_json = root["expressions"][i];
            if (!expression_json.is_object())
            {
                throw ConfigError("expressions[" + std::to_string(i) + "] must be an object");
            }
            if (!expression_json.contains("name") || !expression_json["name"].is_string())
            {
                throw ConfigError("expressions[" + std::to_string(i) + "]: name must be a string");
            }

            const std::string name = expression_json["name"].get<std::string>();
            RequireNonEmptyName("expressions[" + std::to_string(i) + "]", name);
            config.expressions.push_back(ParseExpressionObject(name, expression_json));
        }
        return;
    }

    if (!root["expressions"].is_object())
    {
        throw ConfigError("expressions must be an array or object");
    }

    for (Json::const_iterator it = root["expressions"].begin(); it != root["expressions"].end(); ++it)
    {
        RequireNonEmptyName("expression '" + it.key() + "'", it.key());
        config.expressions.push_back(ParseExpressionObject(it.key(), it.value()));
    }
}

inline int ParseFunctionType(const Json& function_json, std::size_t index)
{
    if (!function_json.contains("function_type") ||
        (!function_json["function_type"].is_number_integer() && !function_json["function_type"].is_number_unsigned()))
    {
        throw ConfigError("functions[" + std::to_string(index) + "]: function_type must be an integer");
    }

    if (function_json["function_type"].is_number_unsigned())
    {
        const std::uint64_t value = function_json["function_type"].get<std::uint64_t>();
        if (value > 2)
        {
            throw ConfigError("functions[" + std::to_string(index) + "]: function_type unsupported '" +
                              std::to_string(value) + "'");
        }
        return static_cast<int>(value);
    }

    const std::int64_t value = function_json["function_type"].get<std::int64_t>();
    if (value < 0 || value > 2)
    {
        throw ConfigError("functions[" + std::to_string(index) + "]: function_type unsupported '" +
                          std::to_string(value) + "'");
    }
    return static_cast<int>(value);
}

inline void ParseFunctionDefinition(const Json& function_json,
                                    std::size_t index,
                                    ExpressionRuntimeConfig& config,
                                    std::unordered_map<std::string, std::string>& raw_constants)
{
    if (!function_json.is_object())
    {
        throw ConfigError("functions[" + std::to_string(index) + "] must be an object");
    }
    if (!function_json.contains("name") || !function_json["name"].is_string())
    {
        throw ConfigError("functions[" + std::to_string(index) + "]: name must be a string");
    }
    const std::string name = function_json["name"].get<std::string>();
    RequireNonEmptyName("functions[" + std::to_string(index) + "]", name);
    const int function_type = ParseFunctionType(function_json, index);

    if (function_type == 0)
    {
        if (!function_json.contains("value") || !function_json["value"].is_string())
        {
            throw ConfigError("function '" + name + "': value must be a string expression");
        }
        AddConstant(name, function_json["value"].get<std::string>(), raw_constants);
        return;
    }

    if (function_type == 1)
    {
        config.tables.push_back(ParseTableObject(name, function_json));
        return;
    }

    if (function_type == 2)
    {
        config.expressions.push_back(ParseExpressionObject(name, function_json));
        return;
    }

    throw ConfigError("functions[" + std::to_string(index) + "]: function_type unsupported '" +
                      std::to_string(function_type) + "'");
}

inline void ParseFunctions(const Json& root,
                           ExpressionRuntimeConfig& config,
                           std::unordered_map<std::string, std::string>& raw_constants)
{
    if (!root.contains("functions"))
    {
        return;
    }
    if (!root["functions"].is_array())
    {
        throw ConfigError("functions must be an array");
    }

    for (std::size_t i = 0; i < root["functions"].size(); ++i)
    {
        ParseFunctionDefinition(root["functions"][i], i, config, raw_constants);
    }
}

} // namespace

inline ExpressionRuntimeConfig LoadExpressionRuntimeConfigFromJsonString(const std::string& json_text)
{
    Json root;
    try
    {
        root = Json::parse(json_text);
    }
    catch (const std::exception& error)
    {
        throw ConfigError(std::string("invalid JSON: ") + error.what());
    }

    return LoadExpressionRuntimeConfigFromJsonObject(root);
}

inline ExpressionRuntimeConfig LoadExpressionRuntimeConfigFromJsonObject(const nlohmann::json& json_object)
{
    if (!json_object.is_object())
    {
        throw ConfigError("root JSON value must be an object");
    }

    // The loader normalizes each top-level section into one internal config
    // object so later compilation stages can stay independent from JSON types.
    ExpressionRuntimeConfig config;
    std::unordered_map<std::string, std::string> raw_constants;
    try
    {
        ParseConstants(json_object, raw_constants);
        ParseTables(json_object, config);
        ParseExpressions(json_object, config);
        ParseFunctions(json_object, config, raw_constants);
        config.constants = ResolveConstants(raw_constants);
    }
    catch (const ExpressionError&)
    {
        throw;
    }
    catch (const nlohmann::json::exception& error)
    {
        throw ConfigError(std::string("invalid JSON config value: ") + error.what());
    }
    return config;
}

inline ExpressionRuntimeConfig LoadExpressionRuntimeConfigFromJsonFile(const std::string& path)
{
    std::ifstream input(path.c_str());
    if (!input)
    {
        throw ConfigError("failed to open JSON file '" + path + "'");
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return LoadExpressionRuntimeConfigFromJsonString(buffer.str());
}

} // namespace utility
} // namespace tfp


namespace tfp
{
namespace utility
{

inline ExpressionHandle::ExpressionHandle() = default;

inline ExpressionHandle::ExpressionHandle(std::shared_ptr<detail::RuntimeExpression> expression)
    : expression_(std::move(expression))
{
}

inline double ExpressionHandle::Evaluate(const std::vector<double>& args) const
{
    if (!expression_)
    {
        throw ExpressionError(ExpressionErrorCode::InvalidArgument, "expression handle is empty");
    }
    return expression_->Evaluate(args);
}

inline std::size_t ExpressionHandle::Arity() const
{
    if (!expression_)
    {
        throw ExpressionError(ExpressionErrorCode::InvalidArgument, "expression handle is empty");
    }
    return expression_->Arity();
}

inline UnaryExpressionHandle::UnaryExpressionHandle() = default;

inline UnaryExpressionHandle::UnaryExpressionHandle(std::shared_ptr<detail::RuntimeExpression> expression)
    : expression_(std::move(expression))
{
}

inline double UnaryExpressionHandle::Evaluate(double x) const
{
    if (!expression_)
    {
        throw ExpressionError(ExpressionErrorCode::InvalidArgument, "unary expression handle is empty");
    }
    return expression_->EvaluateUnary(x);
}

} // namespace utility
} // namespace tfp


namespace tfp
{
namespace utility
{
namespace detail
{

class ExpressionRuntimeImpl
{
public:
    void Load(const ExpressionRuntimeConfig& config)
    {
        // All top-level symbol names share one parser namespace, so conflicts
        // must be rejected before any constants, tables, or expressions are
        // compiled into partially initialized runtime state.
        CheckNameConflicts(config);

        // ExpressionRuntimeConfig stores normalized constants, so compilation
        // can register them directly for tables and runtime expressions.
        std::unordered_map<std::string, double> constants = config.constants;

        std::unordered_map<std::string, std::shared_ptr<const TableFunction> > tables;
        std::unordered_map<std::string, std::vector<std::string> > argument_names;
        for (std::vector<TableConfig>::const_iterator it = config.tables.begin(); it != config.tables.end(); ++it)
        {
            tables[it->name] = std::make_shared<TableFunction>(it->name, it->data, it->extrapolation);
            argument_names[it->name] = std::vector<std::string>{"x"};
        }

        std::unordered_map<std::string, std::shared_ptr<RuntimeExpression> > expressions;
        for (std::vector<ExpressionConfig>::const_iterator it = config.expressions.begin(); it != config.expressions.end(); ++it)
        {
            expressions[it->name] = CompileRuntimeExpression(*it, constants, tables);
            argument_names[it->name] = it->wordable;
        }

        for (std::unordered_map<std::string, double>::const_iterator it = config.constants.begin();
             it != config.constants.end(); ++it)
        {
            argument_names[it->first] = std::vector<std::string>();
        }

        constants_ = std::move(constants);
        tables_ = std::move(tables);
        expressions_ = std::move(expressions);
        argument_names_ = std::move(argument_names);
    }

    ExpressionHandle GetExpression(const std::string& name) const
    {
        return ExpressionHandle(FindExpression(name));
    }

    UnaryExpressionHandle GetUnaryExpression(const std::string& name) const
    {
        std::shared_ptr<RuntimeExpression> expression = FindExpression(name);
        // The same compiled expression representation supports both generic and
        // unary handles, so arity is enforced when the specialized handle is
        // requested rather than during the shared compile pipeline.
        if (expression->Arity() != 1)
        {
            throw ExpressionError(ExpressionErrorCode::InvalidArgument,
                                  "expression '" + name + "' is not unary");
        }
        return UnaryExpressionHandle(expression);
    }

    std::vector<std::string> GetArgumentNames(const std::string& name) const
    {
        return FindArgumentNames(name);
    }

    double Evaluate(const std::string& name, const std::unordered_map<std::string, double>& variables) const
    {
        std::unordered_map<std::string, std::shared_ptr<RuntimeExpression> >::const_iterator expression =
            expressions_.find(name);
        if (expression != expressions_.end())
        {
            return expression->second->EvaluateMap(variables);
        }

        std::unordered_map<std::string, std::shared_ptr<const TableFunction> >::const_iterator table =
            tables_.find(name);
        if (table != tables_.end())
        {
            if (variables.empty())
            {
                throw ExpressionError(ExpressionErrorCode::InvalidArgument,
                                      "table '" + name + "' requires variable 'x'");
            }
            if (variables.size() != 1)
            {
                throw ExpressionError(ExpressionErrorCode::InvalidArgument,
                                      "table '" + name + "' expects exactly one variable");
            }

            return table->second->Evaluate(variables.begin()->second);
        }

        std::unordered_map<std::string, double>::const_iterator constant = constants_.find(name);
        if (constant != constants_.end())
        {
            if (!variables.empty())
            {
                throw ExpressionError(ExpressionErrorCode::InvalidArgument,
                                      "constant '" + name + "' expects no variables");
            }
            return constant->second;
        }

        throw ExpressionError(ExpressionErrorCode::NotFound, "runtime item '" + name + "' does not exist");
    }

    double EvaluateUnary(const std::string& name, double x) const
    {
        std::unordered_map<std::string, std::shared_ptr<RuntimeExpression> >::const_iterator expression =
            expressions_.find(name);
        if (expression != expressions_.end())
        {
            if (expression->second->Arity() != 1)
            {
                throw ExpressionError(ExpressionErrorCode::InvalidArgument,
                                      "expression '" + name + "' is not unary");
            }
            return expression->second->EvaluateUnary(x);
        }

        std::unordered_map<std::string, std::shared_ptr<const TableFunction> >::const_iterator table =
            tables_.find(name);
        if (table != tables_.end())
        {
            return table->second->Evaluate(x);
        }

        std::unordered_map<std::string, double>::const_iterator constant = constants_.find(name);
        if (constant != constants_.end())
        {
            return constant->second;
        }

        throw ExpressionError(ExpressionErrorCode::NotFound, "runtime item '" + name + "' does not exist");
    }

private:
    static void CheckNameConflicts(const ExpressionRuntimeConfig& config)
    {
        // Constants, tables, and expressions are all registered into one
        // global symbol namespace, so duplicate names would become ambiguous
        // long before evaluation.
        std::set<std::string> names;
        for (std::unordered_map<std::string, double>::const_iterator it = config.constants.begin();
             it != config.constants.end(); ++it)
        {
            if (it->first.empty())
            {
                throw ExpressionError(ExpressionErrorCode::ConfigError, "runtime item name must not be empty");
            }
            if (!names.insert(it->first).second)
            {
                throw ExpressionError(ExpressionErrorCode::ConfigError, "duplicate global symbol '" + it->first + "'");
            }
        }
        for (std::vector<TableConfig>::const_iterator it = config.tables.begin(); it != config.tables.end(); ++it)
        {
            if (it->name.empty())
            {
                throw ExpressionError(ExpressionErrorCode::ConfigError, "runtime item name must not be empty");
            }
            if (!names.insert(it->name).second)
            {
                throw ExpressionError(ExpressionErrorCode::ConfigError, "duplicate global symbol '" + it->name + "'");
            }
        }
        for (std::vector<ExpressionConfig>::const_iterator it = config.expressions.begin(); it != config.expressions.end(); ++it)
        {
            if (it->name.empty())
            {
                throw ExpressionError(ExpressionErrorCode::ConfigError, "runtime item name must not be empty");
            }
            if (!names.insert(it->name).second)
            {
                throw ExpressionError(ExpressionErrorCode::ConfigError, "duplicate global symbol '" + it->name + "'");
            }

            std::set<std::string> wordable_names;
            for (std::vector<std::string>::const_iterator wordable = it->wordable.begin();
                 wordable != it->wordable.end(); ++wordable)
            {
                if (!wordable_names.insert(*wordable).second)
                {
                    throw ExpressionError(ExpressionErrorCode::ConfigError,
                                          "expression '" + it->name + "' has duplicate wordable variable '" + *wordable + "'");
                }
            }
        }
    }

    std::shared_ptr<RuntimeExpression> FindExpression(const std::string& name) const
    {
        std::unordered_map<std::string, std::shared_ptr<RuntimeExpression> >::const_iterator it = expressions_.find(name);
        if (it == expressions_.end())
        {
            throw ExpressionError(ExpressionErrorCode::NotFound, "expression '" + name + "' does not exist");
        }
        return it->second;
    }

    std::vector<std::string> FindArgumentNames(const std::string& name) const
    {
        std::unordered_map<std::string, std::vector<std::string> >::const_iterator it = argument_names_.find(name);
        if (it == argument_names_.end())
        {
            throw ExpressionError(ExpressionErrorCode::NotFound, "item '" + name + "' does not exist");
        }
        return it->second;
    }

    std::unordered_map<std::string, double> constants_;
    std::unordered_map<std::string, std::shared_ptr<const TableFunction> > tables_;
    std::unordered_map<std::string, std::shared_ptr<RuntimeExpression> > expressions_;
    std::unordered_map<std::string, std::vector<std::string> > argument_names_;
};

} // namespace detail

inline ExpressionRuntime::ExpressionRuntime()
    : impl_(new detail::ExpressionRuntimeImpl())
{
}

inline ExpressionRuntime::~ExpressionRuntime() = default;
inline ExpressionRuntime::ExpressionRuntime(ExpressionRuntime&&) noexcept = default;
inline ExpressionRuntime& ExpressionRuntime::operator=(ExpressionRuntime&&) noexcept = default;

inline ExpressionRuntime ExpressionRuntime::CreateFromConfig(const ExpressionRuntimeConfig& config)
{
    ExpressionRuntime runtime;
    runtime.LoadFromConfig(config);
    return runtime;
}

inline ExpressionRuntime ExpressionRuntime::CreateFromJsonString(const std::string& json_text)
{
    ExpressionRuntime runtime;
    runtime.LoadFromJsonString(json_text);
    return runtime;
}

inline ExpressionRuntime ExpressionRuntime::CreateFromJsonObject(const nlohmann::json& json_object)
{
    ExpressionRuntime runtime;
    runtime.LoadFromJsonObject(json_object);
    return runtime;
}

inline ExpressionRuntime ExpressionRuntime::CreateFromJsonFile(const std::string& path)
{
    ExpressionRuntime runtime;
    runtime.LoadFromJsonFile(path);
    return runtime;
}

inline void ExpressionRuntime::LoadFromConfig(const ExpressionRuntimeConfig& config)
{
    impl_->Load(config);
}

inline void ExpressionRuntime::LoadFromJsonString(const std::string& json_text)
{
    LoadFromConfig(LoadExpressionRuntimeConfigFromJsonString(json_text));
}

inline void ExpressionRuntime::LoadFromJsonObject(const nlohmann::json& json_object)
{
    LoadFromConfig(LoadExpressionRuntimeConfigFromJsonObject(json_object));
}

inline void ExpressionRuntime::LoadFromJsonFile(const std::string& path)
{
    LoadFromConfig(LoadExpressionRuntimeConfigFromJsonFile(path));
}

inline ExpressionHandle ExpressionRuntime::GetExpression(const std::string& name) const
{
    return impl_->GetExpression(name);
}

inline UnaryExpressionHandle ExpressionRuntime::GetUnaryExpression(const std::string& name) const
{
    return impl_->GetUnaryExpression(name);
}

inline std::vector<std::string> ExpressionRuntime::GetArgumentNames(const std::string& name) const
{
    return impl_->GetArgumentNames(name);
}

inline double ExpressionRuntime::Evaluate(const std::string& name, const std::unordered_map<std::string, double>& variables) const
{
    return impl_->Evaluate(name, variables);
}

inline double ExpressionRuntime::EvaluateUnary(const std::string& name, double x) const
{
    return impl_->EvaluateUnary(name, x);
}

} // namespace utility
} // namespace tfp


#endif
