/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include <string.h>
#include "collector/collector-backend.h"
#include "../provider.h"
#include "dummy-backend.h"

typedef struct dummy_context {
  int dummy_member;
    /* ... */
} dummy_context;

static collector_return_t dummy_create_metric(
        collector_provider_t provider,
        void** context)
{
    (void)provider;

    dummy_context* ctx = (dummy_context*)calloc(1, sizeof(*ctx));
    *context = (void*)ctx;
    return COLLECTOR_SUCCESS;
}

static collector_return_t dummy_open_metric(
        collector_provider_t provider,
        void** context)
{
    (void)provider;

    dummy_context* ctx = (dummy_context*)calloc(1, sizeof(*ctx));
    *context = (void*)ctx;
    return COLLECTOR_SUCCESS;
}

static collector_return_t dummy_close_metric(void* ctx)
{
    dummy_context* context = (dummy_context*)ctx;
    free(context);
    return COLLECTOR_SUCCESS;
}

static collector_return_t dummy_destroy_metric(void* ctx)
{
    dummy_context* context = (dummy_context*)ctx;
    free(context);
    return COLLECTOR_SUCCESS;
}

static void dummy_say_hello(void* ctx)
{
    dummy_context* context = (dummy_context*)ctx;
    (void)context;
    printf("Hello World from Dummy metric\n");
}

static int32_t dummy_compute_sum(void* ctx, int32_t x, int32_t y)
{
    (void)ctx;
    return x+y;
}

static collector_backend_impl dummy_backend = {
    .name             = "dummy",

    .create_metric  = dummy_create_metric,
    .open_metric    = dummy_open_metric,
    .close_metric   = dummy_close_metric,
    .destroy_metric = dummy_destroy_metric,

    .hello            = dummy_say_hello,
    .sum              = dummy_compute_sum
};

collector_return_t collector_provider_register_dummy_backend(collector_provider_t provider)
{
    return collector_provider_register_backend(provider, &dummy_backend);
}
