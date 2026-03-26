# Add the STM32 headers to the project library.
# That way the compiler can know where to find the header files.
# These are then later linked in.
# Still it ignores the warnings on the include dirs by using SYSTEM INTERFACE

# This also allows to later fake the libraries in testing by using some conditional 
# compilation


if(NOT STM32_HEADERS_INCLUDED)
set( STM32_HEADERS_INCLUDED TRUE)

add_library(STM32_HEADERS INTERFACE)

# These are the include dirs of the variable: MX_Include_Dirs
# from the STM32Cube generated cmake file.
target_include_directories(STM32_HEADERS SYSTEM INTERFACE
    ${HMT_PROJECT_LIB_DIR}/../Core/Inc
    ${HMT_PROJECT_LIB_DIR}/../Drivers/STM32F4xx_HAL_Driver/Inc
    ${HMT_PROJECT_LIB_DIR}/../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
    ${HMT_PROJECT_LIB_DIR}/../Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${HMT_PROJECT_LIB_DIR}/../Drivers/CMSIS/Include
)

# Also copied from the STM32Cube cmake file.
target_compile_definitions(STM32_HEADERS INTERFACE
	USE_HAL_DRIVER 
	STM32F446xx
)

endif()