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
#include "led.h"
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

/* USER CODE BEGIN PRIVATE_DEFINES */
#define USB_FLASH_START_ADDRESS (0x8018000)
#define MSC_IMAGE_SIZE (4 * 1024)
#define MSC_BLOCK_SIZE 512
#define MSC_BLOCK_COUNT (MSC_IMAGE_SIZE / MSC_BLOCK_SIZE)
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
uint8_t fat12_disk[MSC_IMAGE_SIZE] = {
    // Boot sector (sector 0)
    0xEB, 0x3C, 0x90,                       // JMP instruction
    'M', 'S', 'D', 'O', 'S', '5', '.', '0', // OEM Name
    0x00, 0x02,                             // Bytes per sector = 512
    0x01,                                   // Sectors per cluster
    0x01, 0x00,                             // Reserved sectors
    0x01,                                   // Number of FATs
    0x10, 0x00,                             // Max root dir entries (16)
    0x40, 0x00,                             // Total sectors (64 * 512 = 32k)
    0xF0,                                   // Media descriptor
    0x01, 0x00,                             // Sectors per FAT
    0x01, 0x00,                             // Sectors per track
    0x01, 0x00,                             // Number of heads
    0x00, 0x00, 0x00, 0x00,                 // Hidden sectors
    0x00, 0x00, 0x00, 0x00,                 // Total sectors (if > 65535)
    // Drive number, reserved, boot sig
    0x00, 0x00, 0x29,
    0x12, 0x34, 0x56, 0x78,                 // Volume ID
    'G', 'i', 'U', 'C', 'A', 'N', ' ', ' ', // Volume label
    'F', 'A', 'T', '1', '2', ' ', ' ', ' ', // File system type
    // Padding to fill sector
    [62] = 0x55,
    [63] = 0xAA, // Boot sector signature
};
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
// Add FAT tables, root dir, and content in initialization code
// For brevity, we only fill a root dir entry here:
void fat12_init(void)
{
  // FAT tables (sector 1)
  fat12_disk[512 + 0] = 0xF0; // Media descriptor
  fat12_disk[512 + 1] = 0xFF;
  fat12_disk[512 + 2] = 0xFF;

  // Root dir entry (sector 2)
  const char *filename = "README  TXT";
  memcpy(&fat12_disk[1024], filename, 11); // Filename
  fat12_disk[1024 + 11] = 0x20;            // File attr: Archive
  fat12_disk[1024 + 26] = 0x03;            // Start cluster
  fat12_disk[1024 + 28] = 0x0C;            // File size (12 bytes)

  // Data sector (cluster 3 = sector 4)
  memcpy(&fat12_disk[512 * 4], "Hello World\n", 12);

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
  *block_num = MSC_BLOCK_COUNT - 1;
  *block_size = MSC_BLOCK_SIZE;
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
  memcpy(buf, &fat12_disk[blk_addr * MSC_BLOCK_SIZE], blk_len * MSC_BLOCK_SIZE);
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
  memcpy(&fat12_disk[blk_addr * MSC_BLOCK_SIZE], buf, blk_len * MSC_BLOCK_SIZE);
  return (USBD_OK);
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
