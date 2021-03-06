/*
 * (C) 2020 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#include "types.h"
#include "admin.h"
#include "collector/collector-admin.h"

collector_return_t collector_admin_init(margo_instance_id mid, collector_admin_t* admin)
{
    collector_admin_t a = (collector_admin_t)calloc(1, sizeof(*a));
    if(!a) return COLLECTOR_ERR_ALLOCATION;

    a->mid = mid;

    *admin = a;
    return COLLECTOR_SUCCESS;
}

collector_return_t collector_admin_finalize(collector_admin_t admin)
{
    free(admin);
    return COLLECTOR_SUCCESS;
}
