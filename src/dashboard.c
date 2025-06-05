#include "dashboard.h"

#ifdef XCAN
static const DashboardItemType types[] = {
#define X(name, str) name,
    DASHBOARD_ITEMS
#undef X
};

static const uint8_t types_count = sizeof(types);

const DashboardItemType type_of(uint8_t index)
{
    if (index < types_count)
    {
        return types[index];
    }
    else
    {
        return UNKNOWN_ITEM;
    }
}

#else
static const uint8_t types_count = 0;
#endif

uint8_t count_dashboard_items(void)
{
    return types_count;
}

#ifdef BHCAN

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
float extractHP(GlobalState *s, uint8_t *_)
{
    return (float)s->car.torque - 500 * (float)s->car.rpm * 0.000142378f;
}
static CarValueExtractors hpExtractors = {.hasV0 = true, .hasV1 = false, .forV0 = {
                                                                             .needsQuery = false,
                                                                             .extract = extractHP,
                                                                         }};

float extractNM(GlobalState *s, uint8_t *_)
{
    return (float)s->car.torque - 500;
}
static CarValueExtractors nmExtractors = {.hasV0 = true, .hasV1 = false, .forV0 = {
                                                                             .needsQuery = false,
                                                                             .extract = extractNM,
                                                                         }};

float extractDpfClog(GlobalState *_, uint8_t *r)
{
    //((A*256)+B)*(1000/65535)
    return ((float)((A(r) * 256) + B(r))) * 0.01525902f;
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
float extractDpfTemp(GlobalState *_, uint8_t *r)
{
    //(((A*256)+B)*0.02)-40
    return ((float)(((A(r) * 256) + B(r))) * 0.02f) - 40.0f;
}

static CarValueExtractors dpfTempExtractors = {.hasV0 = true, .hasV1 = false, .forV0 = {
                                                                                  .needsQuery = true,
                                                                                  .query = {
                                                                                      .reqId = 0x18DA10F1,
                                                                                      .reqData = SWAP_UINT32(0x032218DE),
                                                                                      .replyId = 0x18DAF110,
                                                                                  },
                                                                                  .extract = extractDpfTemp,
                                                                              }};
float extractOilPressure(GlobalState *s, uint8_t *_)
{
    return s->car.oil.pressure;
}

static CarValueExtractors oilPressExtractors = {.hasV0 = true, .hasV1 = false, .forV0 = {
                                                                                   .needsQuery = false,
                                                                                   .extract = extractOilPressure,
                                                                               }};
float extractGear(GlobalState *s, uint8_t *_)
{
    return (float)s->car.gear;
}

static CarValueExtractors gearExtractors = {.hasV0 = true, .hasV1 = false, .forV0 = {
                                                                               .needsQuery = false,
                                                                               .extract = extractGear,
                                                                           }};
CarValueExtractors extractor_of(DashboardItemType type, GlobalState *state)
{
    switch (type)
    {
    case FIRMWARE_ITEM:
        return noExtractors;
    case HP_ITEM:
        return hpExtractors;
    case TORQUE_ITEM:
        return nmExtractors;
    case DPF_CLOG_ITEM:
        return dpfClogExtractors;
    case DPF_TEMP_ITEM:
        return dpfTempExtractors;
    case DPF_REG_ITEM:
        return noExtractors;
    case DPF_DIST_ITEM:
        return noExtractors;
    case DPF_COUNT_ITEM:
        return noExtractors;
    case DPF_MEAN_DIST_ITEM:
        return noExtractors;
    case DPF_MEAN_DURATION_ITEM:
        return noExtractors;
    case BATTERY_V_ITEM:
        return noExtractors;
    case BATTERY_P_ITEM:
        return noExtractors;
    case BATTERY_A_ITEM:
        return noExtractors;
    case OIL_QUALITY_ITEM:
        return noExtractors;
    case OIL_TEMP_ITEM:
        return noExtractors;
    case OIL_PRESS_ITEM:
        return oilPressExtractors;
    case AIR_IN_ITEM:
        return noExtractors;
    case GEAR_ITEM:
        return gearExtractors;
    case GEARBOX_TEMP_ITEM:
        return noExtractors;
    case FRONT_TIRES_TEMP_ITEM:
        return noExtractors;
    case REAR_TIRES_TEMP:
        return noExtractors;
    default:
        return noExtractors;
    }
}
#endif
