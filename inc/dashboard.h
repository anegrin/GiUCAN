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

typedef float (*ExtractionFuncPtr)(GlobalState *state, uint8_t *rx_msg_data);

typedef struct
{
    uint32_t reqId;
    uint32_t reqData;
    uint32_t replyId;
} CANQuery;

typedef struct
{
    bool needsQuery;
    CANQuery query;
    ExtractionFuncPtr extract;
} CarValueExtractor;

typedef struct
{
    bool hasV0;
    bool hasV1;
    CarValueExtractor forV0;
    CarValueExtractor forV1;

} CarValueExtractors;

#ifdef C1CAN
CarValueExtractors extractor_of(DashboardItemType type, GlobalState *state);
#endif

#endif // _DASHBOARD_H
