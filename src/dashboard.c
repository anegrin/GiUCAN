#include "dashboard.h"

#ifdef BHCAN
const char *patterns[] = {
#define X(name, str) str,
    DASHBOARD_ITEMS
#undef X
};

char *pattern_of(DashboardItemType type)
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
