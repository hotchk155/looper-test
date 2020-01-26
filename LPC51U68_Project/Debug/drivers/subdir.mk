################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../drivers/fsl_clock.c \
../drivers/fsl_common.c \
../drivers/fsl_dma.c \
../drivers/fsl_flexcomm.c \
../drivers/fsl_gpio.c \
../drivers/fsl_i2s.c \
../drivers/fsl_i2s_dma.c \
../drivers/fsl_power.c \
../drivers/fsl_reset.c \
../drivers/fsl_spi.c \
../drivers/fsl_spi_dma.c \
../drivers/fsl_usart.c 

OBJS += \
./drivers/fsl_clock.o \
./drivers/fsl_common.o \
./drivers/fsl_dma.o \
./drivers/fsl_flexcomm.o \
./drivers/fsl_gpio.o \
./drivers/fsl_i2s.o \
./drivers/fsl_i2s_dma.o \
./drivers/fsl_power.o \
./drivers/fsl_reset.o \
./drivers/fsl_spi.o \
./drivers/fsl_spi_dma.o \
./drivers/fsl_usart.o 

C_DEPS += \
./drivers/fsl_clock.d \
./drivers/fsl_common.d \
./drivers/fsl_dma.d \
./drivers/fsl_flexcomm.d \
./drivers/fsl_gpio.d \
./drivers/fsl_i2s.d \
./drivers/fsl_i2s_dma.d \
./drivers/fsl_power.d \
./drivers/fsl_reset.d \
./drivers/fsl_spi.d \
./drivers/fsl_spi_dma.d \
./drivers/fsl_usart.d 


# Each subdirectory must supply rules for building sources it contributes
drivers/%.o: ../drivers/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__NEWLIB__ -DCPU_LPC51U68JBD64 -DCPU_LPC51U68JBD64_cm0plus -DFSL_RTOS_BM -DSDK_OS_BAREMETAL -DSERIAL_PORT_TYPE_UART=1 -DSDK_DEBUGCONSOLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"D:\GitHub\looper-test\LPC51U68_Project\board" -I"D:\GitHub\looper-test\LPC51U68_Project\source" -I"D:\GitHub\looper-test\LPC51U68_Project" -I"D:\GitHub\looper-test\LPC51U68_Project\drivers" -I"D:\GitHub\looper-test\LPC51U68_Project\device" -I"D:\GitHub\looper-test\LPC51U68_Project\CMSIS" -I"D:\GitHub\looper-test\LPC51U68_Project\component\uart" -I"D:\GitHub\looper-test\LPC51U68_Project\utilities" -I"D:\GitHub\looper-test\LPC51U68_Project\component\serial_manager" -I"D:\GitHub\looper-test\LPC51U68_Project\component\lists" -O0 -fno-common -g3 -Wall -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="../$(@D)/"=. -mcpu=cortex-m0plus -mthumb -D__NEWLIB__ -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


