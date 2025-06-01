#ifndef _DASHBOARD_H
#define _DASHBOARD_H

#include "config.h"

typedef enum
{
#define X(name, str) name,
    DASHBOARD_ITEMS
#undef X
        DASHBOARD_ITEM_COUNT
} DashboardItemType;

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

static CarValueExtractors noExtractors = {.dynamicV0 = false, .dynamicV1 = false, .v0 = 0.0f, .v1 = 0.0f};
static CarValueExtractors dpfClogExtractors = {.dynamicV0 = true, .dynamicV1 = false, .forV0 = {
                                                                                          .reqId = 0x18DA10F1,
                                                                                          .reqData = SWAP_UINT32(0x032218E4),
                                                                                          .replyId = 0x18DAF110,
                                                                                          .replyLen = 2,
                                                                                          .replyOffset = 0,
                                                                                          .replyValOffset = 0,
                                                                                          .replyScale = 0.015259022,
                                                                                          .replyScaleOffset = 0,
                                                                                      },
                                               .v1 = 0.0f};
CarValueExtractors extractor_of(DashboardItemType type, GlobalState *state)
{
    switch (type)
    {
    case HP_ITEM:
        CarValueExtractors e = {.dynamicV0 = false, .dynamicV1 = false, .forV0 = {0}, .forV1 = {0}, .v0 = state->car.power.hp, .v1 = 0.0f};
        return e;
        break;
    case TORQUE_ITEM:
        CarValueExtractors e = {.dynamicV0 = false, .dynamicV1 = false, .forV0 = {0}, .forV1 = {0}, .v0 = state->car.power.nm, .v1 = 0.0f};
        return e;
        break;
    case DPF_CLOG_ITEM:
        return dpfClogExtractors;
        break;
    default:
        return noExtractors;
        break;
    }
}

#endif // _DASHBOARD_H
