#include "private/Model.h"
#include <stdexcept>

namespace GPIO
{
    std::string model_name(Model model)
    {
        switch (model)
        {
        case CLARA_AGX_XAVIER:
            return "CLARA_AGX_XAVIER";
        case JETSON_NX:
            return "JETSON_NX";
        case JETSON_XAVIER:
            return "JETSON_XAVIER";
        case JETSON_TX1:
            return "JETSON_TX1";
        case JETSON_TX2:
            return "JETSON_TX2";
        case JETSON_NANO:
            return "JETSON_NANO";
        case JETSON_TX2_NX:
            return "JETSON_TX2_NX";
        case JETSON_ORIN:
            return "JETSON_ORIN";
        default:
            throw std::runtime_error("model_name error");
        }
    }
} // namespace GPIO