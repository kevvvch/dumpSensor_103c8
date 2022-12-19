################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/ch4SensorManager.c \
../Core/Src/dumpSensorManager.c \
../Core/Src/fsmManager.c \
../Core/Src/gsmModuleManager.c \
../Core/Src/main.c \
../Core/Src/nh3SensorManager.c \
../Core/Src/nvmManager.c \
../Core/Src/powerModeManager.c \
../Core/Src/softTimer.c \
../Core/Src/stm32f1xx_hal_msp.c \
../Core/Src/stm32f1xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32f1xx.c \
../Core/Src/tempSensorManager.c \
../Core/Src/usSensorManager.c \
../Core/Src/utilities.c 

OBJS += \
./Core/Src/ch4SensorManager.o \
./Core/Src/dumpSensorManager.o \
./Core/Src/fsmManager.o \
./Core/Src/gsmModuleManager.o \
./Core/Src/main.o \
./Core/Src/nh3SensorManager.o \
./Core/Src/nvmManager.o \
./Core/Src/powerModeManager.o \
./Core/Src/softTimer.o \
./Core/Src/stm32f1xx_hal_msp.o \
./Core/Src/stm32f1xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32f1xx.o \
./Core/Src/tempSensorManager.o \
./Core/Src/usSensorManager.o \
./Core/Src/utilities.o 

C_DEPS += \
./Core/Src/ch4SensorManager.d \
./Core/Src/dumpSensorManager.d \
./Core/Src/fsmManager.d \
./Core/Src/gsmModuleManager.d \
./Core/Src/main.d \
./Core/Src/nh3SensorManager.d \
./Core/Src/nvmManager.d \
./Core/Src/powerModeManager.d \
./Core/Src/softTimer.d \
./Core/Src/stm32f1xx_hal_msp.d \
./Core/Src/stm32f1xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32f1xx.d \
./Core/Src/tempSensorManager.d \
./Core/Src/usSensorManager.d \
./Core/Src/utilities.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

