# Generate Model.h file
set(JETSON_MODELS CLARA_AGX_XAVIER JETSON_NX JETSON_XAVIER JETSON_TX2 JETSON_TX1 JETSON_NANO JETSON_TX2_NX JETSON_ORIN JETSON_ORIN_NX JETSON_ORIN_NANO)
set(MODEL_HEADER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/include/private/Model.h)

add_custom_command(
    OUTPUT ${MODEL_HEADER_FILE}
    COMMAND ${CMAKE_COMMAND} -E echo "Running generate_model_header.sh..."
    COMMAND chmod +x ${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_model_header.sh
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_model_header.sh ${MODEL_HEADER_FILE} ${JETSON_MODELS}
    COMMAND ${CMAKE_COMMAND} -E echo "Model.h file has been created."
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateModelHeader.cmake ${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_model_header.sh
    COMMENT "Generating Model.h"
)

add_custom_target(GenerateModelHeader DEPENDS ${MODEL_HEADER_FILE})
add_dependencies(JetsonGPIO GenerateModelHeader)
