/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include <bedrock/module.h>
#include "collector/collector-server.h"
#include "collector/collector-client.h"
#include "collector/collector-admin.h"
#include "collector/collector-provider-handle.h"
#include "client.h"
#include <string.h>

static int collector_register_provider(
        bedrock_args_t args,
        bedrock_module_provider_t* provider)
{
    margo_instance_id mid = bedrock_args_get_margo_instance(args);
    uint16_t provider_id  = bedrock_args_get_provider_id(args);

    struct collector_provider_args collector_args = { 0 };
    collector_args.config = bedrock_args_get_config(args);
    collector_args.pool   = bedrock_args_get_pool(args);

    collector_args.abtio = (abt_io_instance_id)
        bedrock_args_get_dependency(args, "abt_io", 0);

    return collector_provider_register(mid, provider_id, &collector_args,
                                   (collector_provider_t*)provider);
}

static int collector_deregister_provider(
        bedrock_module_provider_t provider)
{
    return collector_provider_destroy((collector_provider_t)provider);
}

static char* collector_get_provider_config(
        bedrock_module_provider_t provider) {
    (void)provider;
    // TODO
    return strdup("{}");
}

static int collector_init_client(
        margo_instance_id mid,
        bedrock_module_client_t* client)
{
    return collector_client_init(mid, (collector_client_t*)client);
}

static int collector_finalize_client(
        bedrock_module_client_t client)
{
    return collector_client_finalize((collector_client_t)client);
}

static int collector_create_provider_handle(
        bedrock_module_client_t client,
        hg_addr_t address,
        uint16_t provider_id,
        bedrock_module_provider_handle_t* ph)
{
    collector_client_t c = (collector_client_t)client;
    collector_provider_handle_t tmp = calloc(1, sizeof(*tmp));
    margo_addr_dup(c->mid, address, &(tmp->addr));
    tmp->provider_id = provider_id;
    *ph = (bedrock_module_provider_handle_t)tmp;
    return BEDROCK_SUCCESS;
}

static int collector_destroy_provider_handle(
        bedrock_module_provider_handle_t ph)
{
    collector_provider_handle_t tmp = (collector_provider_handle_t)ph;
    margo_addr_free(tmp->mid, tmp->addr);
    free(tmp);
    return BEDROCK_SUCCESS;
}

static struct bedrock_module collector = {
    .register_provider       = collector_register_provider,
    .deregister_provider     = collector_deregister_provider,
    .get_provider_config     = collector_get_provider_config,
    .init_client             = collector_init_client,
    .finalize_client         = collector_finalize_client,
    .create_provider_handle  = collector_create_provider_handle,
    .destroy_provider_handle = collector_destroy_provider_handle,
    .dependencies            = NULL
};

BEDROCK_REGISTER_MODULE(collector, collector)
