#include "dashboard.h"

#ifdef BHCAN
const char *patterns[] = {
#define X(name, str) str,
    DASHBOARD_ITEMS
#undef X
};

const char *pattern_of(DashboardItemType type)
{
    switch (type)
    {
#define X(name, str) \
    case name:       \
        return str;
        DASHBOARD_ITEMS
#undef X
    default:
        return "<unknown>";
    }
}
#endif

#ifdef C1CAN
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
        CarValueExtractors eHp = {.dynamicV0 = false, .dynamicV1 = false, .forV0 = {0}, .forV1 = {0}, .v0 = state->car.power.hp, .v1 = 0.0f};
        return eHp;
        break;
    case TORQUE_ITEM:
        CarValueExtractors eT = {.dynamicV0 = false, .dynamicV1 = false, .forV0 = {0}, .forV1 = {0}, .v0 = state->car.power.nm, .v1 = 0.0f};
        return eT;
        break;
    case DPF_CLOG_ITEM:
        return dpfClogExtractors;
        break;
    default:
        return noExtractors;
        break;
    }
}
#endif
