################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/Tasks/task_can.c \
../Core/Src/Tasks/task_display.c \
../Core/Src/Tasks/task_errors.c \
../Core/Src/Tasks/task_inputs.c 

OBJS += \
./Core/Src/Tasks/task_can.o \
./Core/Src/Tasks/task_display.o \
./Core/Src/Tasks/task_errors.o \
./Core/Src/Tasks/task_inputs.o 

C_DEPS += \
./Core/Src/Tasks/task_can.d \
./Core/Src/Tasks/task_display.d \
./Core/Src/Tasks/task_errors.d \
./Core/Src/Tasks/task_inputs.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/Tasks/%.o Core/Src/Tasks/%.su Core/Src/Tasks/%.cyclo: ../Core/Src/Tasks/%.c Core/Src/Tasks/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F303x8 -c -I../Core/Inc -I"C:/Users/zinga/Documents/.UNIVPM/PolimarcheRacingTeam/Volante/dashboard-peacockElettrica/dashboard-stm32/Core/Src/Tasks" -I"C:/Users/zinga/Documents/.UNIVPM/PolimarcheRacingTeam/Volante/dashboard-peacockElettrica/dashboard-stm32/Core/Src/Communication" -I"C:/Users/zinga/Documents/.UNIVPM/PolimarcheRacingTeam/Volante/dashboard-peacockElettrica/dashboard-stm32/Core/Inc/Communication" -I../Drivers/STM32F3xx_HAL_Driver/Inc -I../Drivers/STM32F3xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F3xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-Tasks

clean-Core-2f-Src-2f-Tasks:
	-$(RM) ./Core/Src/Tasks/task_can.cyclo ./Core/Src/Tasks/task_can.d ./Core/Src/Tasks/task_can.o ./Core/Src/Tasks/task_can.su ./Core/Src/Tasks/task_display.cyclo ./Core/Src/Tasks/task_display.d ./Core/Src/Tasks/task_display.o ./Core/Src/Tasks/task_display.su ./Core/Src/Tasks/task_errors.cyclo ./Core/Src/Tasks/task_errors.d ./Core/Src/Tasks/task_errors.o ./Core/Src/Tasks/task_errors.su ./Core/Src/Tasks/task_inputs.cyclo ./Core/Src/Tasks/task_inputs.d ./Core/Src/Tasks/task_inputs.o ./Core/Src/Tasks/task_inputs.su

.PHONY: clean-Core-2f-Src-2f-Tasks

