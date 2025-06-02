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
static CarValueExtractors noExtractors = {.hasV0 = false, .hasV1 = false};
static CarValueExtractors dpfClogExtractors = {.hasV0 = true, .hasV1 = false, .forV0 = {
                                                                                  .needsQuery = true,
                                                                                  .reqId = 0x18DA10F1,
                                                                                  .reqData = SWAP_UINT32(0x032218E4),
                                                                                  .replyId = 0x18DAF110,
                                                                                  .replyLen = 2,
                                                                                  .replyOffset = 0,
                                                                                  .replyValOffset = 0,
                                                                                  .replyScale = 0.015259022,
                                                                                  .replyScaleOffset = 0,
                                                                              }};
CarValueExtractors extractor_of(DashboardItemType type, GlobalState *state)
{
    switch (type)
    {
    case HP_ITEM:
        CarValueExtractors eHp = {.hasV0 = true, .hasV1 = false, .forV0 = {0}, .forV1 = {.needsQuery = false, .value = (float)state->car.power.hp}};
        return eHp;
        break;
    case TORQUE_ITEM:
        CarValueExtractors eT = {.hasV0 = true, .hasV1 = false, .forV0 = {0}, .forV1 = {.needsQuery = false, .value = (float)state->car.power.nm}};
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
