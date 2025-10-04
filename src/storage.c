#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "storage.h"
#include "dashboard.h"
#include "logging.h"
#define INI_ALLOW_MULTILINE 0
#define INI_ALLOW_BOM 0
#define INI_ALLOW_INLINE_COMMENTS 0
#define INI_START_COMMENT_PREFIXES 0
#include "ini.h"
#include "config.h"
#include "ff.h"

void storage_init(void)
{
#ifdef ENABLE_USB_MASS_STORAGE
    FATFS fs;
    FIL fil;
    UINT bw;
    FRESULT res;
    BYTE work[FF_MIN_SS];

    res = f_mount(&fs, "", 1);
    if (res != FR_OK)
    {
        MKFS_PARM opt = {.fmt = FM_FAT | FM_SFD, .n_fat = 1, .align = 0, .n_root = 32, .au_size = FF_MIN_SS};
        res = f_mkfs("", &opt, work, FF_MIN_SS);
        if (res == FR_OK)
        {
            res = f_setlabel("GIUCAN");
            res = f_open(&fil, "giucan.ver", FA_WRITE | FA_OPEN_ALWAYS);
            if (res == FR_OK)
            {
                res = f_write(&fil, GIUCAN_VERSION, strlen(GIUCAN_VERSION), &bw);
                f_close(&fil);
            }
        }
    }

    res = f_unmount("");
#endif
}

void init_settings(Settings *settings)
{
    settings->bootCarouselEnabled = false;
    settings->bootCarouselDelay = 10000;
    settings->bootCarouselInterval = 3000;
    settings->bootCarouselLoops = 3;
    uint8_t NO_ITEMS[0];
    settings->favoriteItems = NO_ITEMS;
    settings->favoriteItemsCount = 0;
}

#ifdef ENABLE_RW_USB_MASS_STORAGE

void validate_and_fix_settings(Settings *settings)
{

    if (settings->bootCarouselEnabled)
    {
#define ENSURE_BOUNDARIES(p, min, max) \
    if (p < min)                       \
    {                                  \
        p = min;                       \
    }                                  \
    else if (p > max)                  \
    {                                  \
        p = max;                       \
    }
        ENSURE_BOUNDARIES(settings->bootCarouselDelay, 5000, 60000)
        ENSURE_BOUNDARIES(settings->bootCarouselInterval, 1000, 20000)
        ENSURE_BOUNDARIES(settings->bootCarouselLoops, 1, 5)
    }
}

typedef struct FavItemNode
{
    uint8_t item;
    struct FavItemNode *next;
} FavItemNode;

static int settings_ini_handler(void *user, const char *section, const char *name,
                                const char *value)
{
    Settings *settings = (Settings *)user;

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    if (MATCH("carousel", "delayMs"))
    {
        settings->bootCarouselDelay = atoi(value);
    }
    else if (MATCH("carousel", "intervalMs"))
    {
        settings->bootCarouselInterval = atoi(value);
    }
    else if (MATCH("carousel", "loops"))
    {
        settings->bootCarouselLoops = atoi(value);
    }
    else if (MATCH("carousel", "enabled"))
    {
        settings->bootCarouselEnabled = strcmp("true", value) == 0 ? true : false;
    }
    else if (MATCH("dashboard", "favorites"))
    {
        const char *delims = " ,";
        char *split_me = malloc(strlen(value));
        strcpy(split_me, value);
        char *token = strtok(split_me, delims);
        FavItemNode *fin = NULL;
        FavItemNode *fin_head = NULL;
        uint8_t count = 0;
        while (token != NULL)
        {
            if (fin)
            {
                FavItemNode *next_fin = malloc(sizeof(FavItemNode));
                fin->next = next_fin;
                fin = next_fin;
            }
            else
            {
                fin = malloc(sizeof(FavItemNode));
                fin_head = fin;
            }

            uint8_t item = atoi(token);
            fin->item = item;
            fin->next = NULL;
            token = strtok(NULL, delims);
            count++;
        }
        free(split_me);

        settings->favoriteItems = malloc(count);

        FavItemNode *free_me_iter = fin_head;
        int i = 0;
        while (free_me_iter)
        {

            settings->favoriteItems[i++] = free_me_iter->item;
            settings->favoriteItemsCount = i;
            FavItemNode *temp = free_me_iter;
            free_me_iter = free_me_iter->next;
            free(temp);
        }
    }
    else
    {
        return 0;
    }

    return 1;
}

void load_settings(Settings *settings)
{
    init_settings(settings);
    FATFS fs;
    FIL fil;
    FRESULT res;

    res = f_mount(&fs, "", 1);
    if (res == FR_OK)
    {
        res = f_open(&fil, "settings.ini", FA_READ);
        if (res == FR_OK)
        {
            FSIZE_t fileSize = f_size(&fil);

            char *ini = malloc(sizeof(char) * fileSize + 1);

            if (ini)
            {
                UINT br;
                res = f_read(&fil, ini, fileSize, &br);
                if (res == FR_OK && br >= fileSize)
                {
                    ini[fileSize] = '\0';
                    int err = ini_parse_string(ini, settings_ini_handler, settings);
                    if (err)
                    {
                        init_settings(settings);
                    }
                    else
                    {
                        validate_and_fix_settings(settings);
                    }
                }
                free(ini);
            }

            f_close(&fil);
        }
    }

    res = f_unmount("");
}
#else
void load_settings(Settings *settings)
{
    init_settings(settings);
}
#endif
