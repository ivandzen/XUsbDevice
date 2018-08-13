/*
 * XUsbDevice_Config.h
 *
 *  Created on: Sep 3, 2017
 *      Author: ivan
 */

#ifndef XUSBDEVICE_CONFIG_H_
#define XUSBDEVICE_CONFIG_H_
#include <stm32f7xx_hal.h>

#define USB_MAX_EP0_SIZE 	64
#define USB_MAX_CONFIGS		2
#define USB_MAX_STRINGS		16
#define USB_MAX_INTERFACES 	4

#define HAL_XUsbDevice_SetAddress(handle, addr) \
		HAL_PCD_SetAddress((PCD_HandleTypeDef*)handle, dev_addr)

#define HAL_XUsbDevice_Transmit(handle, ep_addr, pbuf, size) \
		HAL_PCD_EP_Transmit((PCD_HandleTypeDef*)handle, ep_addr, pbuf, size)

#define HAL_XUsbDevice_Receive(handle, ep_addr, pbuf, size) \
		HAL_PCD_EP_Receive((PCD_HandleTypeDef*)handle, ep_addr, pbuf, size)

#define HAL_XUsbDevice_StallEP(handle, epnum) \
		HAL_PCD_EP_SetStall((PCD_HandleTypeDef*)handle, epnum)

#define HAL_XUsbDevice_OpenEP(handle, ep_addr, ep_mps, ep_type) \
		HAL_PCD_EP_Open((PCD_HandleTypeDef*)(handle), ep_addr, ep_mps, ep_type)

#define HAL_XUsbDevice_CloseEP(handle, ep_addr) \
		HAL_PCD_EP_Close((PCD_HandleTypeDef*)handle, ep_addr)

#define HAL_XUsbDevice_FlushEP(handle, ep_addr) \
		HAL_PCD_EP_Flush((PCD_HandleTypeDef*)handle, ep_addr)

#define HAL_XUsbDevice_ClearStallEP(handle, epnum) \
		HAL_PCD_EP_ClrStall((PCD_HandleTypeDef*)handle, epnum)

#define HAL_XUsbDevice_GetRxCount(handle, ep_addr) \
		HAL_PCD_EP_GetRxCount((PCD_HandleTypeDef*)handle, ep_addr)

#define HAL_XUsbDevice_GetState(handle) \
		HAL_PCD_GetState((PCD_HandleTypeDef*)handle)

#define HAL_XUsbDevice_IsStallEP(handle, ep_addr) \
	((((ep_addr) & 0x80) == 0x80) ? \
	(((PCD_HandleTypeDef*)handle)->IN_ep[(ep_addr) & 0x7F].is_stall) :\
	(((PCD_HandleTypeDef*)handle)->OUT_ep[(ep_addr) & 0x7F].is_stall))

#endif /* XUSBDEVICE_CONFIG_H_ */
