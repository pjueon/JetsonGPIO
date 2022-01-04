#include <string>
#include <stdexcept>
#include "PythonFunctions.h"

namespace GPIO
{
    // for exception handling
    static std::string _error_message(const std::exception& e, const std::string& from)
    {
        return format("[Exception] %s (catched from: %s)\n", e.what(), from.c_str());
    }

    static void _rethrow_exception(const std::exception& e, const std::string& from)
    {
        throw std::runtime_error(_error_message(e, from));
    }
}
