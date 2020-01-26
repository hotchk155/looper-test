################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../source/LPC51U68_Project.cpp \
../source/cpp_config.cpp 

C_SRCS += \
../source/semihost_hardfault.c 

OBJS += \
./source/LPC51U68_Project.o \
./source/cpp_config.o \
./source/semihost_hardfault.o 

CPP_DEPS += \
./source/LPC51U68_Project.d \
./source/cpp_config.d 

C_DEPS += \
./source/semihost_hardfault.d 


# Each subdirectory must supply rules for building sources it contributes
source/%.o: ../source/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C++ Compiler'
	arm-none-eabi-c++ -DCPU_LPC51U68JBD64 -DCPU_LPC51U68JBD64_cm0plus -DFSL_RTOS_BM -DSDK_OS_BAREMETAL -DSERIAL_PORT_TYPE_UART=1 -DSDK_DEBUGCONSOLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__NEWLIB__ -I"D:\GitHub\looper-test\LPC51U68_Project\board" -I"D:\GitHub\looper-test\LPC51U68_Project\source" -I"D:\GitHub\looper-test\LPC51U68_Project" -I"D:\GitHub\looper-test\LPC51U68_Project\drivers" -I"D:\GitHub\looper-test\LPC51U68_Project\device" -I"D:\GitHub\looper-test\LPC51U68_Project\CMSIS" -I"D:\GitHub\looper-test\LPC51U68_Project\component\uart" -I"D:\GitHub\looper-test\LPC51U68_Project\utilities" -I"D:\GitHub\looper-test\LPC51U68_Project\component\serial_manager" -I"D:\GitHub\looper-test\LPC51U68_Project\component\lists" -O0 -fno-common -g3 -Wall -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fno-rtti -fno-exceptions -fmerge-constants -fmacro-prefix-map="../$(@D)/"=. -mcpu=cortex-m0plus -mthumb -D__NEWLIB__ -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

source/%.o: ../source/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__NEWLIB__ -DCPU_LPC51U68JBD64 -DCPU_LPC51U68JBD64_cm0plus -DFSL_RTOS_BM -DSDK_OS_BAREMETAL -DSERIAL_PORT_TYPE_UART=1 -DSDK_DEBUGCONSOLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"D:\GitHub\looper-test\LPC51U68_Project\board" -I"D:\GitHub\looper-test\LPC51U68_Project\source" -I"D:\GitHub\looper-test\LPC51U68_Project" -I"D:\GitHub\looper-test\LPC51U68_Project\drivers" -I"D:\GitHub\looper-test\LPC51U68_Project\device" -I"D:\GitHub\looper-test\LPC51U68_Project\CMSIS" -I"D:\GitHub\looper-test\LPC51U68_Project\component\uart" -I"D:\GitHub\looper-test\LPC51U68_Project\utilities" -I"D:\GitHub\looper-test\LPC51U68_Project\component\serial_manager" -I"D:\GitHub\looper-test\LPC51U68_Project\component\lists" -O0 -fno-common -g3 -Wall -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="../$(@D)/"=. -mcpu=cortex-m0plus -mthumb -D__NEWLIB__ -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


