################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../startup/startup_lpc51u68.cpp 

OBJS += \
./startup/startup_lpc51u68.o 

CPP_DEPS += \
./startup/startup_lpc51u68.d 


# Each subdirectory must supply rules for building sources it contributes
startup/%.o: ../startup/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C++ Compiler'
	arm-none-eabi-c++ -DCPU_LPC51U68JBD64 -DCPU_LPC51U68JBD64_cm0plus -DFSL_RTOS_BM -DSDK_OS_BAREMETAL -DSERIAL_PORT_TYPE_UART=1 -DSDK_DEBUGCONSOLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__NEWLIB__ -I"D:\GitHub\looper-test\LPC51U68_Project\board" -I"D:\GitHub\looper-test\LPC51U68_Project\source" -I"D:\GitHub\looper-test\LPC51U68_Project" -I"D:\GitHub\looper-test\LPC51U68_Project\drivers" -I"D:\GitHub\looper-test\LPC51U68_Project\device" -I"D:\GitHub\looper-test\LPC51U68_Project\CMSIS" -I"D:\GitHub\looper-test\LPC51U68_Project\component\uart" -I"D:\GitHub\looper-test\LPC51U68_Project\utilities" -I"D:\GitHub\looper-test\LPC51U68_Project\component\serial_manager" -I"D:\GitHub\looper-test\LPC51U68_Project\component\lists" -O0 -fno-common -g3 -Wall -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fno-rtti -fno-exceptions -fmerge-constants -fmacro-prefix-map="../$(@D)/"=. -mcpu=cortex-m0plus -mthumb -D__NEWLIB__ -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


