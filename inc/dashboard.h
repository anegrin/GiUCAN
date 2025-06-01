#ifndef _DASHBOARD_H
#define _DASHBOARD_H

#include <stdbool.h>
#include "config.h"
#include "model.h"

#ifdef XCAN
typedef enum
{
#define X(name, str) name,
    DASHBOARD_ITEMS
#undef X
        DASHBOARD_ITEM_COUNT
} DashboardItemType;
#else
#define DASHBOARD_ITEM_COUNT 0
#endif

#ifdef BHCAN
const char *pattern_of(DashboardItemType type);
#endif

#define SWAP_UINT32(x) (((uint32_t)(x) >> 24) & 0x000000FF) |    \
                           (((uint32_t)(x) >> 8) & 0x0000FF00) | \
                           (((uint32_t)(x) << 8) & 0x00FF0000) | \
                           (((uint32_t)(x) << 24) & 0xFF000000)

typedef struct
{
    uint32_t reqId;
    uint8_t reqLen;
    uint32_t reqData;
    uint32_t replyId;
    uint8_t replyLen;
    uint8_t replyOffset;
    int32_t replyValOffset;
    float replyScale;
    int32_t replyScaleOffset;

} CarValueExtractor;

typedef struct
{
    bool dynamicV0;
    bool dynamicV1;
    CarValueExtractor forV0;
    CarValueExtractor forV1;
    float v0;
    float v1;

} CarValueExtractors;

#ifdef C1CAN
CarValueExtractors extractor_of(DashboardItemType type, GlobalState *state);
#endif

#endif // _DASHBOARD_H
