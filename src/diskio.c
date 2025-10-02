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
#include "logging.h"
#include <string.h>
#include <stdlib.h>

#define PAGE_SIZE 2048
#define SECTORS_PER_PAGE PAGE_SIZE / FF_MIN_SS

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

typedef struct FlashPage
{
    struct FlashPage *next;
    uint32_t page_start;
    LBA_t sectors[SECTORS_PER_PAGE];
    uint32_t sector_data_index[SECTORS_PER_PAGE];
} FlashPage;

#ifdef DEBUG_MODE
void print_flash_list(FlashPage *head)
{
    FlashPage *current = head;
    while (current)
    {
        LOG("FlashPage start: %x\n", current->page_start);
        LOGS("  Sectors: ");
        for (int i = 0; i < SECTORS_PER_PAGE; i++)
        {
            LOG("%x:%d ", current->sectors[i], current->sector_data_index[i]);
        }
        LOGS("\n\n");
        current = current->next;
    }
}
#endif

FlashPage* new_flash_page()
{
    FlashPage *fp = (FlashPage *)malloc(sizeof(FlashPage));
    for (int j = 0; j < SECTORS_PER_PAGE; j++)
    {
        fp->sectors[j] = 0;
        fp->sector_data_index[j] = 0;
    }
    fp->page_start = 0;
    fp->next = NULL;
    return fp;
}

uint32_t find_sector_start(LBA_t sector) {
    return USB_FLASH_START_ADDRESS + (sector * FF_MIN_SS);
} 

uint32_t find_page_start(LBA_t sector) {
    uint32_t sector_start = find_sector_start(sector);
    uint32_t delta =  sector_start % PAGE_SIZE;
    return sector_start - delta;
} 

DRESULT disk_write(
    BYTE pdrv,        /* Physical drive nmuber to identify the drive */
    const BYTE *buff, /* Data to be written */
    LBA_t sector,     /* Start sector in LBA */
    UINT count        /* Number of sectors to write */
)
{
    HAL_StatusTypeDef ret = HAL_OK;

    // build operations
    FlashPage *fp = NULL;
    FlashPage *fp_head = NULL;
    for (int i = 0; i < count; i++)
    {
        LBA_t current_sector = sector + i;
        uint32_t page_address = find_page_start(current_sector);
        uint32_t sector_address = find_sector_start(current_sector);
        uint8_t sector_relative_pos = (sector_address - page_address) / FF_MIN_SS;

        if (fp == NULL)
        {
            fp = new_flash_page();
            fp_head = fp;
        }
        else
        {
            if (fp->page_start != page_address)
            {
                FlashPage *new_fp = new_flash_page();
                fp->next = new_fp;
                fp = new_fp;
            }
        }

        fp->page_start = page_address;
        fp->sectors[sector_relative_pos] = sector_address;
        fp->sector_data_index[sector_relative_pos] = i * FF_MIN_SS;
    }

#ifdef DEBUG_MODE
    print_flash_list(fp_head);
#endif

    FlashPage *iter = fp_head;
    while (iter)
    {
        ret = HAL_FLASH_Unlock();
        if (ret != HAL_OK)
        {
            ret = RES_NOTRDY;
            break;
        }

        uint8_t page_data[PAGE_SIZE];
        memcpy(page_data, (const void *)(iter->page_start), PAGE_SIZE);
        LOG("memcpy(page_data, (const void *)(%x), PAGE_SIZE)\n", iter->page_start);
        for (int i = 0; i<SECTORS_PER_PAGE; i++){
            
            uint32_t sector_address = iter->sectors[i];
            if (sector_address) {
                memcpy(&page_data[i * FF_MIN_SS], &buff[iter->sector_data_index[i]], FF_MIN_SS);
                LOG("memcpy(&page_data[%d], &buff[%d], FF_MIN_SS)\n", i * FF_MIN_SS, iter->sector_data_index[i]);
            }

        }

        FLASH_EraseInitTypeDef EraseInitStruct;
        uint32_t SectorError;
        EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
        EraseInitStruct.PageAddress = iter->page_start;
        EraseInitStruct.NbPages = 1;

        ret = HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError);
        if (ret != HAL_OK)
        {
            HAL_FLASH_Lock();
            ret = RES_NOTRDY;
            break;
        }

        for (uint32_t j = 0; j < PAGE_SIZE / 4; j++)
        {
            uint32_t word;
            memcpy(&word, &page_data[j * 4], 4);

            ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, iter->page_start + (j * 4), word);
            if (ret != HAL_OK)
            {
                break;
            }
        }

        HAL_FLASH_Lock();

        iter = iter->next;
    }

    FlashPage *free_me_iter = fp_head;
    while (free_me_iter)
    {
        FlashPage *temp = free_me_iter;
        free_me_iter = free_me_iter->next;
        free(temp);
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
        *((WORD*)buff) = (TOTAL_USB_DEVICE_SIZE / FF_MIN_SS);
        return RES_OK;
    case GET_SECTOR_SIZE:
        *((WORD*)buff) = FF_MIN_SS;
        return RES_OK;
    case GET_BLOCK_SIZE:
        *((WORD*)buff) = (PAGE_SIZE / FF_MIN_SS);
        return RES_OK;
    default:
        return RES_PARERR;
    }
}
