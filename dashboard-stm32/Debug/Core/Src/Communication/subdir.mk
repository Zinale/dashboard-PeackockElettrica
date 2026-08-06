################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/Communication/can.c \
../Core/Src/Communication/serial.c 

OBJS += \
./Core/Src/Communication/can.o \
./Core/Src/Communication/serial.o 

C_DEPS += \
./Core/Src/Communication/can.d \
./Core/Src/Communication/serial.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/Communication/%.o Core/Src/Communication/%.su Core/Src/Communication/%.cyclo: ../Core/Src/Communication/%.c Core/Src/Communication/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F303x8 -c -I../Core/Inc -I"E:/UNIVPM/PolimarcheRacingTeam/Volante/dashboard-peacockElettrica/dashboard-stm32/Core/Src/Tasks" -I"E:/UNIVPM/PolimarcheRacingTeam/Volante/dashboard-peacockElettrica/dashboard-stm32/Core/Src/Communication" -I"E:/UNIVPM/PolimarcheRacingTeam/Volante/dashboard-peacockElettrica/dashboard-stm32/Core/Inc/Communication" -I../Drivers/STM32F3xx_HAL_Driver/Inc -I../Drivers/STM32F3xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F3xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-Communication

clean-Core-2f-Src-2f-Communication:
	-$(RM) ./Core/Src/Communication/can.cyclo ./Core/Src/Communication/can.d ./Core/Src/Communication/can.o ./Core/Src/Communication/can.su ./Core/Src/Communication/serial.cyclo ./Core/Src/Communication/serial.d ./Core/Src/Communication/serial.o ./Core/Src/Communication/serial.su

.PHONY: clean-Core-2f-Src-2f-Communication

