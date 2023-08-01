// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText 2023 Kristin Ebuengan
// SPDX-FileCopyrightText 2023 Melody Gill
// SPDX-FileCopyrightText 2023 Gabriel Marcano

/*
 * Gets data from the photo resistor using the ADC
 * and prints the voltage and resistance from the sensor
*/

#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"

#include <string.h>
#include <assert.h>

#include <uart.h>
#include <adc.h>
#include <math.h>

static struct uart uart;
static struct adc adc;

int main(void)
{
	// Prepare MCU by init-ing clock, cache, and power level operation
	am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_SYSCLK_MAX, 0);
	am_hal_cachectrl_config(&am_hal_cachectrl_defaults);
	am_hal_cachectrl_enable();
	am_bsp_low_power_init();
	am_hal_sysctrl_fpu_enable();
	am_hal_sysctrl_fpu_stacking_enable(true);

	// Init UART
	uart_init(&uart, UART_INST0);

	// Initialize the ADC.
	uint8_t pins[2];
	pins[0] = 16;
	pins[1] = 29;
	size_t size = 2;
	adc_init(&adc, pins, 2);

	// After init is done, enable interrupts
	am_hal_interrupt_master_enable();

	// Wait here for the ISR to grab a buffer of samples.
	while (1)
	{
		// Print the battery voltage and the resistance of the photo resister
		// (to know how bright the ambient environment is)
		// Trigger the ADC to start collecting data
		adc_trigger(&adc);
		uint32_t data[1][3];
		if (adc_get_sample(&adc, data, pins, size))
		{
			// The math here is straight forward: we've asked the ADC to give
			// us data in 14 bits (max value of 2^14 -1). We also specified the
			// reference voltage to be 1.5V. A reading of 1.5V would be
			// translated to the maximum value of 2^14-1. So we divide the
			// value from the ADC by this maximum, and multiply it by the
			// reference, which then gives us the actual voltage measured.
			const double reference = 1.5;
			double voltage = data[0][0] * reference / ((1 << 14) - 1);
			am_util_stdio_printf(
				"voltage = <%.3f> (0x%04X) ", voltage, data[0][0]);
			am_util_stdio_printf("\r\n");
			double resistance = (10000 * voltage)/(3.3 - voltage);
			am_util_stdio_printf(
				"resistance = <%.3f> ", resistance);
			am_util_stdio_printf("\r\n\r\n");
			voltage = data[0][1] * reference / ((1 << 14) - 1);
			am_util_stdio_printf(
				"internal voltage = <%.3f> (0x%04X) ", voltage, data[0][1]);
			am_util_stdio_printf("\r\n");
		}
		
		// Sleep here until the next ADC interrupt comes along.
		am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);
	}
}