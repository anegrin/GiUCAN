#include "dashboard.h"
#include "printf.h"

#define SWAP_ENDIAN32(x) (((uint32_t)(x) >> 24) & 0x000000FF) |    \
                             (((uint32_t)(x) >> 8) & 0x0000FF00) | \
                             (((uint32_t)(x) << 8) & 0x00FF0000) | \
                             (((uint32_t)(x) << 24) & 0xFF000000)

#define REQ_RES_ID_CONVERSION(x) ((x & 0xFFFF0000) |        \
                                  ((x & 0x000000FF) << 8) | \
                                  ((x & 0x0000FF00) >> 8))

#ifdef BHCAN
#define X(item_type, forV0_return_type, forV0_convert_function_code, forV1_return_type, forV1_convert_function_code) \
    forV0_return_type item_type##_V0Converter(float value)                                                           \
    {                                                                                                                \
        return forV0_convert_function_code;                                                                          \
    }                                                                                                                \
    forV1_return_type item_type##_V1Converter(float value)                                                           \
    {                                                                                                                \
        return forV1_convert_function_code;                                                                          \
    }
CONVERTERS
#undef X

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

const char *dpf_status_as_string(float value)
{
    switch ((uint8_t)value)
    {
    case 1:
        return "DPF Low";
    case 2:
        return "DPF High";
    case 3:
        return "De-NOx";
    case 4:
        return "De-SOx";
    case 5:
        return "SCR HeatUp";
    default:
        return "Idle";
    }
}

void render_message(char *buffer, GlobalState *state)
{
    DashboardItemType type = state->board.dashboardState.currentItemIndex;
    const char *pattern = pattern_of(type);

    int written = -1;

    switch (type)
    {
#define X(item_type, _1, _2, _3, _4)                                                                                                                                                                 \
    case item_type:                                                                                                                                                                                  \
        written = snprintf_(buffer, DASHBOARD_BUFFER_SIZE, pattern, item_type##_V0Converter(state->board.dashboardState.values[0]), item_type##_V1Converter(state->board.dashboardState.values[1])); \
        break;
        CONVERTERS
#undef X
    default:
        written = snprintf_(buffer, DASHBOARD_BUFFER_SIZE, pattern, state->board.dashboardState.values[0], state->board.dashboardState.values[1]);
    }

    if (written >= 0 && written < DASHBOARD_MESSAGE_MAX_LENGTH)
    {
        memset(buffer + written, ' ', DASHBOARD_MESSAGE_MAX_LENGTH - written);
    }
    buffer[DASHBOARD_MESSAGE_MAX_LENGTH] = 0x00;
}
#endif

#ifdef C1CAN
inline float noop_extract(GlobalState *s, uint8_t *r) { return 0; }
#define X(name, code)                      \
    float name(GlobalState *s, uint8_t *r) \
    {                                      \
        return code;                       \
    }
EXTRACTION_FUNCTIONS
#undef X

static CarValueExtractors noExtractors = {
    .hasV0 = false,
    .hasV1 = false};

#define X(item_type, has_V0, V0_needsQuery, V0_query_reqId, V0_query_reqData, V0_extraction_function, has_V1, V1_needsQuery, V1_query_reqId, V1_query_reqData, V1_extraction_function) \
    static CarValueExtractors item_type##_extractors = {                                                                                                                               \
        .hasV0 = has_V0,                                                                                                                                                               \
        .forV0 = {                                                                                                                                                                     \
            .needsQuery = V0_needsQuery,                                                                                                                                               \
            .query = {                                                                                                                                                                 \
                .reqId = V0_query_reqId,                                                                                                                                               \
                .reqData = SWAP_ENDIAN32(V0_query_reqData),                                                                                                                            \
                .replyId = REQ_RES_ID_CONVERSION(V0_query_reqId),                                                                                                                      \
            },                                                                                                                                                                         \
            .extract = V0_extraction_function,                                                                                                                                         \
        },                                                                                                                                                                             \
        .hasV1 = has_V1,                                                                                                                                                               \
        .forV1 = {                                                                                                                                                                     \
            .needsQuery = V1_needsQuery,                                                                                                                                               \
            .query = {                                                                                                                                                                 \
                .reqId = V1_query_reqId,                                                                                                                                               \
                .reqData = SWAP_ENDIAN32(V1_query_reqData),                                                                                                                            \
                .replyId = REQ_RES_ID_CONVERSION(V1_query_reqId),                                                                                                                      \
            },                                                                                                                                                                         \
            .extract = V1_extraction_function,                                                                                                                                         \
        }};
EXTRACTORS
#undef X

CarValueExtractors extractor_of(DashboardItemType type, GlobalState *state)
{
    switch (type)
    {
#define X(item_type, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10) \
    case item_type:                                           \
        return item_type##_extractors;
        EXTRACTORS
#undef X
    default:
        return noExtractors;
    }
}

uint32_t values_refresh_rate_of(DashboardItemType type)
{
    switch (type)
    {
#define X(item_type, ms) \
    case item_type:      \
        return ms;
        VALUES_REFRESH_MS
#undef X
    default:
        return DEFAULT_VALUES_REFRESH_MS;
    }
}
#endif
