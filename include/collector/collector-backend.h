/*
 * (C) 2020 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __COLLECTOR_BACKEND_H
#define __COLLECTOR_BACKEND_H

#include <collector/collector-server.h>
#include <collector/collector-common.h>

typedef collector_return_t (*collector_backend_create_fn)(collector_provider_t, void**);
typedef collector_return_t (*collector_backend_open_fn)(collector_provider_t, void**);
typedef collector_return_t (*collector_backend_close_fn)(void*);
typedef collector_return_t (*collector_backend_destroy_fn)(void*);

/**
 * @brief Implementation of an COLLECTOR backend.
 */
typedef struct collector_backend_impl {
    // backend name
    const char* name;
    // backend management functions
    collector_backend_create_fn   create_metric;
    collector_backend_open_fn     open_metric;
    collector_backend_close_fn    close_metric;
    collector_backend_destroy_fn  destroy_metric;
    // RPC functions
    void (*hello)(void*);
    int32_t (*sum)(void*, int32_t, int32_t);
    // ... add other functions here
} collector_backend_impl;

/**
 * @brief Registers a backend implementation to be used by the
 * specified provider.
 *
 * Note: the backend implementation will not be copied; it is
 * therefore important that it stays valid in memory until the
 * provider is destroyed.
 *
 * @param provider provider.
 * @param backend_impl backend implementation.
 *
 * @return COLLECTOR_SUCCESS or error code defined in collector-common.h 
 */
collector_return_t collector_provider_register_backend();

#endif
