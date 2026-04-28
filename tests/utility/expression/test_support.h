#ifndef TFP_UTILITY_EXPRESSION_TEST_SUPPORT_H
#define TFP_UTILITY_EXPRESSION_TEST_SUPPORT_H

#include <cmath>
#include <cstdlib>
#include <exception>
#include <functional>
#include <iostream>
#include <string>

namespace tfp
{
namespace utility
{
namespace test
{

inline void Require(bool condition, const char* expression, const char* file, int line)
{
    if (!condition)
    {
        std::cerr << file << ":" << line << ": requirement failed: " << expression << "\n";
        std::exit(1);
    }
}

inline void RequireNear(double actual, double expected, double tolerance, const char* file, int line)
{
    if (std::fabs(actual - expected) > tolerance)
    {
        std::cerr << file << ":" << line << ": expected " << expected << ", got " << actual << "\n";
        std::exit(1);
    }
}

template <typename ExceptionT>
void RequireThrows(const std::function<void()>& function, const char* expression, const char* file, int line)
{
    try
    {
        function();
    }
    catch (const ExceptionT&)
    {
        return;
    }
    catch (const std::exception& error)
    {
        std::cerr << file << ":" << line << ": " << expression << " threw unexpected exception: "
                  << error.what() << "\n";
        std::exit(1);
    }

    std::cerr << file << ":" << line << ": expected exception from " << expression << "\n";
    std::exit(1);
}

} // namespace test
} // namespace utility
} // namespace tfp

#define TFP_REQUIRE(expr) ::tfp::utility::test::Require((expr), #expr, __FILE__, __LINE__)
#define TFP_REQUIRE_NEAR(actual, expected, tolerance) \
    ::tfp::utility::test::RequireNear((actual), (expected), (tolerance), __FILE__, __LINE__)
#define TFP_REQUIRE_THROWS(exception_type, expr) \
    ::tfp::utility::test::RequireThrows<exception_type>([&]() { expr; }, #expr, __FILE__, __LINE__)

#endif
