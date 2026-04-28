#include "tfp/utility/expression/table_function.h"

#include "tfp/utility/expression/expression_error.h"

#include "mpICallback.h"
#include "mpIToken.h"
#include "mpValue.h"

#include <memory>

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

mup::ICallback* CreateTableCallback(const std::shared_ptr<const TableFunction>& table)
{
    return new MuParserXTableCallback(table);
}

} // namespace detail
} // namespace utility
} // namespace tfp
