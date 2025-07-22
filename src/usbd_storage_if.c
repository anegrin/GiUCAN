/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : usbd_storage_if.c
 * @version        : v2.0_Cube
 * @brief          : Memory management layer.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "usbd_storage_if.h"

/* USER CODE BEGIN INCLUDE */
#include <stdbool.h>
/* USER CODE END INCLUDE */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
 * @brief Usb device.
 * @{
 */

/** @defgroup USBD_STORAGE
 * @brief Usb mass storage device module
 * @{
 */

/** @defgroup USBD_STORAGE_Private_TypesDefinitions
 * @brief Private types.
 * @{
 */

/* USER CODE BEGIN PRIVATE_TYPES */

/* USER CODE END PRIVATE_TYPES */

/**
 * @}
 */

/** @defgroup USBD_STORAGE_Private_Defines
 * @brief Private defines.
 * @{
 */

#define STORAGE_LUN_NBR 1
#define STORAGE_BLK_NBR 0x40
#define STORAGE_BLK_SIZ 0x200

/* USER CODE BEGIN PRIVATE_DEFINES */
#define USB_FLASH_START_ADDRESS (0x8018000)
#define TOTAL_USB_DEVICE_SIZE (STORAGE_BLK_NBR * STORAGE_BLK_SIZ)
/* USER CODE END PRIVATE_DEFINES */

/**
 * @}
 */

/** @defgroup USBD_STORAGE_Private_Macros
 * @brief Private macros.
 * @{
 */

/* USER CODE BEGIN PRIVATE_MACRO */

/* USER CODE END PRIVATE_MACRO */

/**
 * @}
 */

/** @defgroup USBD_STORAGE_Private_Variables
 * @brief Private variables.
 * @{
 */

/* USER CODE BEGIN INQUIRY_DATA_FS */
/** USB Mass storage Standard Inquiry Data. */
const int8_t STORAGE_Inquirydata_FS[] = {
    /* 36 */

    /* LUN 0 */
    0x00,
    0x80,
    0x02,
    0x02,
    (STANDARD_INQUIRY_DATA_LEN - 5),
    0x00,
    0x00,
    0x00,
    'T', 'O', 'S', 'C', ' ', ' ', ' ', ' ', /* Manufacturer : 8 bytes */
    'G', 'i', 'U', 'C', 'A', 'N', ' ', 'M', /* Product      : 16 Bytes */
    'S', 'C', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', '0', '.', '1' /* Version      : 4 Bytes */
};
/* USER CODE END INQUIRY_DATA_FS */

/* USER CODE BEGIN PRIVATE_VARIABLES */

/* USER CODE END PRIVATE_VARIABLES */

/**
 * @}
 */

/** @defgroup USBD_STORAGE_Exported_Variables
 * @brief Public variables.
 * @{
 */

extern USBD_HandleTypeDef hUsbDeviceFS;

/* USER CODE BEGIN EXPORTED_VARIABLES */

/* USER CODE END EXPORTED_VARIABLES */

/**
 * @}
 */

/** @defgroup USBD_STORAGE_Private_FunctionPrototypes
 * @brief Private functions declaration.
 * @{
 */

static int8_t STORAGE_Init_FS(uint8_t lun);
static int8_t STORAGE_GetCapacity_FS(uint8_t lun, uint32_t *block_num, uint16_t *block_size);
static int8_t STORAGE_IsReady_FS(uint8_t lun);
static int8_t STORAGE_IsWriteProtected_FS(uint8_t lun);
static int8_t STORAGE_Read_FS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
static int8_t STORAGE_Write_FS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
static int8_t STORAGE_GetMaxLun_FS(void);

/* USER CODE BEGIN PRIVATE_FUNCTIONS_DECLARATION */
/**
 * @brief  Writes data into the FLASH.
 * @param  buf: data buffer.
 * @param  blk_addr: Logical block address.
 * @param  blk_len: Blocks number.
 * @retval HAL_StatusTypeDef
 */
static HAL_StatusTypeDef write_data_to_flash(uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
  HAL_StatusTypeDef ret = HAL_OK;
  uint8_t data[TOTAL_USB_DEVICE_SIZE];
  do
  {
    /* First copy the data to the local buffer from Flash */
    memcpy(data, (const void *)USB_FLASH_START_ADDRESS, TOTAL_USB_DEVICE_SIZE);

    /* Make modifications in the local buffer */
    memcpy((void *)&data[blk_addr * STORAGE_BLK_SIZ], buf, (blk_len * STORAGE_BLK_SIZ));

    ret = HAL_FLASH_Unlock();
    if (ret != HAL_OK)
    {
      break;
    }

    /* Erase the Flash */
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t SectorError;
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = USB_FLASH_START_ADDRESS;
    EraseInitStruct.NbPages = TOTAL_USB_DEVICE_SIZE / 2048;

    ret = HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError);

    /* Write the data to the Flash */
    for (uint32_t i = 0; i < TOTAL_USB_DEVICE_SIZE; i++)
    {
      ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
                              (USB_FLASH_START_ADDRESS + i),
                              data[i]);

      if (ret != HAL_OK)
      {
        break;
      }
    }

    HAL_FLASH_Lock();
  } while (false);
  return (ret);
}
/* USER CODE END PRIVATE_FUNCTIONS_DECLARATION */

/**
 * @}
 */

USBD_StorageTypeDef USBD_Storage_Interface_fops_FS =
    {
        STORAGE_Init_FS,
        STORAGE_GetCapacity_FS,
        STORAGE_IsReady_FS,
        STORAGE_IsWriteProtected_FS,
        STORAGE_Read_FS,
        STORAGE_Write_FS,
        STORAGE_GetMaxLun_FS,
        (int8_t *)STORAGE_Inquirydata_FS};

/* Private functions ---------------------------------------------------------*/
/**
 * @brief  Initializes over USB FS IP
 * @param  lun:
 * @retval USBD_OK if all operations are OK else USBD_FAIL
 */
int8_t STORAGE_Init_FS(uint8_t lun)
{
  /* USER CODE BEGIN 2 */
  return (USBD_OK);
  /* USER CODE END 2 */
}

/**
 * @brief  .
 * @param  lun: .
 * @param  block_num: .
 * @param  block_size: .
 * @retval USBD_OK if all operations are OK else USBD_FAIL
 */
int8_t STORAGE_GetCapacity_FS(uint8_t lun, uint32_t *block_num, uint16_t *block_size)
{
  /* USER CODE BEGIN 3 */
  *block_num = STORAGE_BLK_NBR;
  *block_size = STORAGE_BLK_SIZ;
  return (USBD_OK);
  /* USER CODE END 3 */
}

/**
 * @brief  .
 * @param  lun: .
 * @retval USBD_OK if all operations are OK else USBD_FAIL
 */
int8_t STORAGE_IsReady_FS(uint8_t lun)
{
  /* USER CODE BEGIN 4 */
  return (USBD_OK);
  /* USER CODE END 4 */
}

/**
 * @brief  .
 * @param  lun: .
 * @retval USBD_OK if all operations are OK else USBD_FAIL
 */
int8_t STORAGE_IsWriteProtected_FS(uint8_t lun)
{
  /* USER CODE BEGIN 5 */
  return (USBD_OK);
  /* USER CODE END 5 */
}

/**
 * @brief  .
 * @param  lun: .
 * @retval USBD_OK if all operations are OK else USBD_FAIL
 */
int8_t STORAGE_Read_FS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
  /* USER CODE BEGIN 6 */
  /* We can read directly from the Flash address */
  memcpy( buf,
          (const void *)(USB_FLASH_START_ADDRESS + ( blk_addr * STORAGE_BLK_SIZ )),
          (blk_len * STORAGE_BLK_SIZ)
        );
        
  return (USBD_OK);
  /* USER CODE END 6 */
}

/**
 * @brief  .
 * @param  lun: .
 * @retval USBD_OK if all operations are OK else USBD_FAIL
 */
int8_t STORAGE_Write_FS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
  /* USER CODE BEGIN 7 */
  return write_data_to_flash( buf, blk_addr, blk_len );
  /* USER CODE END 7 */
}

/**
 * @brief  .
 * @param  None
 * @retval .
 */
int8_t STORAGE_GetMaxLun_FS(void)
{
  /* USER CODE BEGIN 8 */
  return (STORAGE_LUN_NBR - 1);
  /* USER CODE END 8 */
}

/* USER CODE BEGIN PRIVATE_FUNCTIONS_IMPLEMENTATION */

/* USER CODE END PRIVATE_FUNCTIONS_IMPLEMENTATION */

/**
 * @}
 */

/**
 * @}
 */
