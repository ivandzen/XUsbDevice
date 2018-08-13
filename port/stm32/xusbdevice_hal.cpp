/*
 * XUsbDevice_HAL.cpp
 *
 *  Created on: Dec 20, 2017
 *      Author: ivan
 */
#include <stm32f7xx_hal.h>
#include <plugins/usb/device/XUsbDevice.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief  Setup stage callback
  * @param  hpcd: PCD handle
  * @retval None
  */
void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd) {
	XUsbDevice * device = (XUsbDevice*)hpcd->pData;
    device->setupStage((uint8_t*)hpcd->Setup);
}

/**
  * @brief  Data Out stage callback.
  * @param  hpcd: PCD handle
  * @param  epnum: Endpoint Number
  * @retval None
  */
void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
	XUsbDevice * device = (XUsbDevice*)hpcd->pData;
    device->dataOutStage(epnum, hpcd->OUT_ep[epnum].xfer_buff);
}

/**
  * @brief  Data In stage callback..
  * @param  hpcd: PCD handle
  * @param  epnum: Endpoint Number
  * @retval None
  */
void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
	XUsbDevice * device = (XUsbDevice*)hpcd->pData;
    device->dataInStage(epnum, hpcd->IN_ep[epnum].xfer_buff);
}

/**
  * @brief  SOF callback.
  * @param  hpcd: PCD handle
  * @retval None
  */
void HAL_PCD_SOFCallback(PCD_HandleTypeDef *hpcd) {
	XUsbDevice * device = (XUsbDevice*)hpcd->pData;
    device->SOF();
}

/**
  * @brief  Reset callback.
  * @param  hpcd: PCD handle
  * @retval None
  */
void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd) {
	XUsbDevice * device = (XUsbDevice*)hpcd->pData;
	XUsbDevice::Speed speed = XUsbDevice::USBD_SPEED_FULL;

    /*Set USB Current Speed*/
    switch (hpcd->Init.speed) {
    case PCD_SPEED_HIGH:
        speed = XUsbDevice::USBD_SPEED_HIGH;
        break;
    case PCD_SPEED_FULL:
        speed = XUsbDevice::USBD_SPEED_FULL;
        break;

    default:
        speed = XUsbDevice::USBD_SPEED_FULL;
        break;
    }

    device->reset(speed);
}

/**
  * @brief  Suspend callback.
  * When Low power mode is enabled the debug cannot be used (IAR, Keil doesn't support it)
  * @param  hpcd: PCD handle
  * @retval None
  */
void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *hpcd)
{
	XUsbDevice * device = (XUsbDevice*)hpcd->pData;
    /* Inform USB library that core enters in suspend Mode */
    device->suspend();
    __HAL_PCD_GATE_PHYCLOCK(hpcd);
    /*Enter in STOP mode */
    /* USER CODE BEGIN 2 */
    if (hpcd->Init.low_power_enable) {
        /* Set SLEEPDEEP bit and SleepOnExit of Cortex System Control Register */
        SCB->SCR |= (uint32_t)((uint32_t)(SCB_SCR_SLEEPDEEP_Msk | SCB_SCR_SLEEPONEXIT_Msk));
    }
    /* USER CODE END 2 */
}

/**
  * @brief  Resume callback.
    When Low power mode is enabled the debug cannot be used (IAR, Keil doesn't support it)
  * @param  hpcd: PCD handle
  * @retval None
  */
void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *hpcd)
{
	XUsbDevice * device = (XUsbDevice*)hpcd->pData;
    device->resume();
}

/**
  * @brief  ISOC Out Incomplete callback.
  * @param  hpcd: PCD handle
  * @param  epnum: Endpoint Number
  * @retval None
  */
void HAL_PCD_ISOOUTIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
	XUsbDevice * device = (XUsbDevice*)hpcd->pData;
    device->isoOutIncomplete(epnum);
}

/**
  * @brief  ISOC In Incomplete callback.
  * @param  hpcd: PCD handle
  * @param  epnum: Endpoint Number
  * @retval None
  */
void HAL_PCD_ISOINIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
	XUsbDevice * device = (XUsbDevice*)hpcd->pData;
    device->isoInIncomplete(epnum);
}

/**
  * @brief  Connect callback.
  * @param  hpcd: PCD handle
  * @retval None
  */
void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *hpcd)
{
	XUsbDevice * device = (XUsbDevice*)hpcd->pData;
    device->connected();
}

/**
  * @brief  Disconnect callback.
  * @param  hpcd: PCD handle
  * @retval None
  */
void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *hpcd)
{
	XUsbDevice * device = (XUsbDevice*)hpcd->pData;
    device->disconnected();
}


#if (USBD_LPM_ENABLED == 1)
/**
  * @brief  HAL_PCDEx_LPM_Callback : Send LPM message to user layer
  * @param  hpcd: PCD handle
  * @param  msg: LPM message
  * @retval HAL status
  */
void HAL_PCDEx_LPM_Callback(PCD_HandleTypeDef *hpcd, PCD_LPM_MsgTypeDef msg) {
	XUsbDevice * device = (XUsbDevice*)hpcd->pData;
    switch ( msg) {
    case PCD_LPM_L0_ACTIVE:
        if (hpcd->Init.low_power_enable) {
            SystemClock_Config();

            /* Reset SLEEPDEEP bit of Cortex System Control Register */
            SCB->SCR &= (uint32_t)~((uint32_t)(SCB_SCR_SLEEPDEEP_Msk | SCB_SCR_SLEEPONEXIT_Msk));
        }
        __HAL_PCD_UNGATE_PHYCLOCK(hpcd);
        device->resume();
        break;

    case PCD_LPM_L1_ACTIVE:
        __HAL_PCD_GATE_PHYCLOCK(hpcd);
        device->suspend();

        /*Enter in STOP mode */
        if (hpcd->Init.low_power_enable) {
            /* Set SLEEPDEEP bit and SleepOnExit of Cortex System Control Register */
            SCB->SCR |= (uint32_t)((uint32_t)(SCB_SCR_SLEEPDEEP_Msk | SCB_SCR_SLEEPONEXIT_Msk));
        }
        break;
  }
}
#endif


#ifdef __cplusplus
}
#endif
