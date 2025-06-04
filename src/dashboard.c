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
float extractHP(GlobalState *state, uint8_t *rx_msg_data)
{
    return (float)state->car.torque - 500 * (float)state->car.rpm * 0.000142378f;
}
static CarValueExtractors hpExtractors = {.hasV0 = true, .hasV1 = false, .forV0 = {
                                                                             .needsQuery = false,
                                                                             .extract = extractHP,
                                                                         }};

float extractNM(GlobalState *state, uint8_t *rx_msg_data)
{
    return (float)state->car.torque - 500;
}
static CarValueExtractors nmExtractors = {.hasV0 = true, .hasV1 = false, .forV0 = {
                                                                             .needsQuery = false,
                                                                             .extract = extractNM,
                                                                         }};

float extractDpfClog(GlobalState *state, uint8_t *rx_msg_data)
{
    //TODO
    //((A*256)+B)*(1000/65535)
    return (float)state->car.rpm / 100;
}

static CarValueExtractors dpfClogExtractors = {.hasV0 = true, .hasV1 = false, .forV0 = {
                                                                                  .needsQuery = true,
                                                                                  .query = {
                                                                                      .reqId = 0x18DA10F1,
                                                                                      .reqData = SWAP_UINT32(0x032218E4),
                                                                                      .replyId = 0x18DAF110,
                                                                                  },
                                                                                  .extract = extractDpfClog,
                                                                              }};
float extractOilPressure(GlobalState *state, uint8_t *rx_msg_data)
{
    return state->car.oil.pressure;
}

static CarValueExtractors oilPressExtractors = {.hasV0 = true, .hasV1 = false, .forV0 = {
                                                                                  .needsQuery = false,
                                                                                  .extract = extractOilPressure,
                                                                              }};
float extractGear(GlobalState *state, uint8_t *rx_msg_data)
{
    return state->car.gear;
}

static CarValueExtractors gearExtractors = {.hasV0 = true, .hasV1 = false, .forV0 = {
                                                                                  .needsQuery = false,
                                                                                  .extract = extractGear,
                                                                              }};
CarValueExtractors extractor_of(DashboardItemType type, GlobalState *state)
{
    switch (type)
    {
    case HP_ITEM:
        return hpExtractors;
        break;
    case TORQUE_ITEM:
        return nmExtractors;
        break;
    case DPF_CLOG_ITEM:
        return dpfClogExtractors;
        break;
    case OIL_PRESS_ITEM:
        return oilPressExtractors;
        break;
    case GEAR_ITEM:
        return gearExtractors;
        break;
    default:
        return noExtractors;
        break;
    }
}
#endif
