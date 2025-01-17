/*
 * system_state.h
 *
 *  Created on: Jan 19, 2023
 *      Author: schaefer
 */

#ifndef INC_SYSTEM_STATE_H_
#define INC_SYSTEM_STATE_H_

enum availability_bits
{
	GNSS_AVAILABLE 		= 1,
	D_GNSS_AVAILABLE 	= 2,

	MTI_SENSOR_AVAILABE 	= 0x10,
	FXOS_SENSOR_AVAILABLE 	= 0x20,
	L3GD20_SENSOR_AVAILABLE = 0x40,
	MS5611_STATIC_AVAILABLE = 0x80,
	MS5611_PITOT_AVAILABLE  = 0x100,
	PITOT_SENSOR_AVAILABLE 	= 0x200,
	AIR_SENSOR_AVAILABLE 	= 0x400,

	USART_1_OUTPUT_ACTIVE	= 0x800,
	USB_OUTPUT_ACTIVE	= 0x1000,
	BLUEZ_OUTPUT_ACTIVE	= 0x2000,
	CAN_OUTPUT_ACTIVE	= 0x4000,
	USART_2_OUTPUT_ACTIVE	= 0x8000
};

extern uint32_t system_state; //!< bits collected from availability_bits

inline void update_system_state_set( unsigned value)
{
#ifndef __MSVC__
  __atomic_or_fetch ( &system_state, value, __ATOMIC_ACQUIRE);
#endif
}

inline void update_system_state_clear( unsigned value)
{
#ifndef __MSVC__
  	__atomic_and_fetch ( &system_state, ~value, __ATOMIC_ACQUIRE);
#endif
}

#endif /* INC_SYSTEM_STATE_H_ */
