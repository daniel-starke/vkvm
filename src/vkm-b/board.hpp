/**
 * @file board.hpp
 * @author Daniel Starke
 * @copyright Copyright 2020-2022 Daniel Starke
 * @date 2020-05-10
 * @version 2023-10-25
 */
#ifndef __VKM_BOARD_HPP__
#define __VKM_BOARD_HPP__

#include <stm32f0xx.h>
#include <stm32f0xx_hal.h>
#include <stm32f0xx_ll_cortex.h>
#include <stm32f0xx_ll_exti.h>
#include <stm32f0xx_ll_gpio.h>
#include <stm32f0xx_ll_system.h>
#include <stm32f0xx_ll_tim.h>


#ifndef __STM32F042x6_H
#error Missing include of stm32f042x6.h. Please define STM32F042x6.
#endif


/** USB endpoint size. */
#define USB_EP_SIZE 64
/** USB reception buffer size. */
#define USB_RX_SIZE (USB_EP_SIZE * 2)
/** USB transmission buffer size. */
#define USB_TX_SIZE (USB_EP_SIZE * 4)


/** USB interrupt priority. */
#define USB_IRQ_PRIO 0
/** USB interrupt sub-priority. */
#define USB_IRQ_SUBPRIO 0

/** URTT interrupt priority. */
#define UART_IRQ_PRIO 1
/** URTT interrupt sub-priority. */
#define UART_IRQ_SUBPRIO 0


#endif /* __VKM_BOARD_HPP__ */
