/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __COLLECTOR_PROVIDER_HANDLE_H
#define __COLLECTOR_PROVIDER_HANDLE_H

#include <margo.h>
#include <collector/collector-common.h>

#ifdef __cplusplus
extern "C" {
#endif

struct collector_provider_handle {
    margo_instance_id mid;
    hg_addr_t         addr;
    uint16_t          provider_id;
};

typedef struct collector_provider_handle* collector_provider_handle_t;
#define COLLECTOR_PROVIDER_HANDLE_NULL ((collector_provider_handle_t)NULL)

#ifdef __cplusplus
}
#endif

#endif
