/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2025        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"     /* Basic definitions of FatFs */
#include "diskio.h" /* Declarations FatFs MAI */

/* Example: Declarations of the platform and disk functions in the project */
#include "config.h"
#include <string.h>

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(
    BYTE pdrv /* Physical drive nmuber to identify the drive */
)
{
    return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize(
    BYTE pdrv /* Physical drive nmuber to identify the drive */
)
{
    return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(
    BYTE pdrv,    /* Physical drive nmuber to identify the drive */
    BYTE *buff,   /* Data buffer to store read data */
    LBA_t sector, /* Start sector in LBA */
    UINT count    /* Number of sectors to read */
)
{
    memcpy(buff,
           (const void *)(USB_FLASH_START_ADDRESS + (sector * FF_MIN_SS)),
           (count * FF_MIN_SS));

    return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write(
    BYTE pdrv,        /* Physical drive nmuber to identify the drive */
    const BYTE *buff, /* Data to be written */
    LBA_t sector,     /* Start sector in LBA */
    UINT count        /* Number of sectors to write */
)
{
    HAL_StatusTypeDef ret = HAL_OK;

    for (int i = 0; i < count; i++)
    {

        uint8_t data[FF_MIN_SS];

        memcpy(&data[0], &buff[i * FF_MIN_SS], FF_MIN_SS);

        ret = HAL_FLASH_Unlock();
        if (ret != HAL_OK)
        {
            break;
        }

        uint32_t pageAddress = USB_FLASH_START_ADDRESS + ((sector + i) * FF_MIN_SS);

        FLASH_EraseInitTypeDef EraseInitStruct;
        uint32_t SectorError;
        EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
        EraseInitStruct.PageAddress = pageAddress;
        EraseInitStruct.NbPages = 1;

        ret = HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError);

        for (uint32_t j = 0; j < FF_MIN_SS / 4; j++)
        {
            uint32_t word;
            memcpy(&word, &data[j * 4], 4);
            ;
            ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, pageAddress + (j * 4), word);
            if (ret != HAL_OK)
            {
                break;
            }
        }
        HAL_FLASH_Lock();
    }

    return ret != HAL_OK ? RES_ERROR : RES_OK;
}

#endif

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl(
    BYTE pdrv, /* Physical drive nmuber (0..) */
    BYTE cmd,  /* Control code */
    void *buff /* Buffer to send/receive control data */
)
{
    switch (cmd)
    {
    case CTRL_TRIM:
    case CTRL_SYNC:
        return RES_OK;
    case GET_SECTOR_COUNT:
        *(DWORD *)buff = (TOTAL_USB_DEVICE_SIZE / FF_MIN_SS);
        return RES_OK;
    case GET_SECTOR_SIZE:
        *(DWORD *)buff = FF_MIN_SS;
        return RES_OK;
    case GET_BLOCK_SIZE:
        *(DWORD *)buff = 1;
        return RES_OK;
    default:
        return RES_PARERR;
    }
}
