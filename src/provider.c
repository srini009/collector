/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include <assert.h>
#include "collector/collector-server.h"
#include "collector/collector-common.h"
#include "collector/collector-backend.h"
#ifdef USE_AGGREGATOR
#include <aggregator/aggregator-provider-handle.h>
#include <aggregator/aggregator-client.h>
#endif
#include "provider.h"
#include "types.h"

static void collector_finalize_provider(void* p);

/* Functions to manipulate the hash of metrics */

static inline collector_metric* find_metric(
        collector_provider_t provider,
        const collector_metric_id_t* id);

static inline collector_return_t add_metric(
        collector_provider_t provider,
        collector_metric* metric);

static inline collector_return_t remove_metric(
        collector_provider_t provider,
        const collector_metric_id_t* id);

static inline void remove_all_metrics(
        collector_provider_t provider);

/* Admin RPCs */

/* Client RPCs */
static DECLARE_MARGO_RPC_HANDLER(collector_metric_fetch_ult)
static void collector_metric_fetch_ult(hg_handle_t h);
static DECLARE_MARGO_RPC_HANDLER(collector_list_metrics_ult)
static void collector_list_metrics_ult(hg_handle_t h);

/* add other RPC declarations here */

int collector_provider_register(
        margo_instance_id mid,
        uint16_t provider_id,
        const struct collector_provider_args* args,
        collector_provider_t* provider)
{
    struct collector_provider_args a = COLLECTOR_PROVIDER_ARGS_INIT;
    if(args) a = *args;
    collector_provider_t p;
    hg_id_t id;
    hg_bool_t flag;

    margo_info(mid, "Registering COLLECTOR provider with provider id %u", provider_id);

    flag = margo_is_listening(mid);
    if(flag == HG_FALSE) {
        margo_error(mid, "Margo instance is not a server");
        return COLLECTOR_ERR_INVALID_ARGS;
    }

    margo_provider_registered_name(mid, "collector_remote_metric_fetch", provider_id, &id, &flag);
    if(flag == HG_TRUE) {
        margo_error(mid, "Provider with the same provider id (%u) already register", provider_id);
        return COLLECTOR_ERR_INVALID_PROVIDER;
    }

    p = (collector_provider_t)calloc(1, sizeof(*p));
    if(p == NULL) {
        margo_error(mid, "Could not allocate memory for provider");
        return COLLECTOR_ERR_ALLOCATION;
    }

    p->mid = mid;
    p->provider_id = provider_id;
    p->pool = a.pool;
    p->abtio = a.abtio;

    /* Admin RPCs */

    /* Client RPCs */
    id = MARGO_REGISTER_PROVIDER(mid, "collector_remote_metric_fetch",
            metric_fetch_in_t, metric_fetch_out_t,
            collector_metric_fetch_ult, provider_id, p->pool);
    margo_register_data(mid, id, (void*)p, NULL);
    p->metric_fetch_id = id;

    id = MARGO_REGISTER_PROVIDER(mid, "collector_remote_list_metrics",
            list_metrics_in_t, list_metrics_out_t,
            collector_list_metrics_ult, provider_id, p->pool);
    margo_register_data(mid, id, (void*)p, NULL);
    p->list_metrics_id = id;
    p->use_aggregator = 0;

    /* add other RPC registration here */
    /* ... */

    /* add backends available at compiler time (e.g. default/dummy backends) */


#ifdef USE_AGGREGATOR
    #define MAXCHAR 100
    FILE *fp_agg = NULL;
    char * aggregator_addr_file = getenv("AGGREGATOR_ADDRESS_FILE");
    if(aggregator_addr_file) {
        char svr_addr_str[MAXCHAR];
        uint16_t p_id;
        fp_agg = fopen(aggregator_addr_file, "r");
        int32_t num_aggregators;
        int i = 0;
        fscanf(fp_agg, "%d\n", num_aggregators);
        aggregator_client_init(mid, &p->aggcl);
        aggregator_provider_handle_t *aggphs = (aggregator_provider_handle_t *)malloc(sizeof(aggregator_provider_handle_t)*num_aggregators);
        while(fscanf(fp_agg, "%s %u\n", svr_addr_str, &p_id) != EOF) {
          hg_addr_t svr_addr; 
          int hret = margo_addr_lookup(mid, svr_addr_str, &svr_addr);
          assert(hret == HG_SUCCESS);
          aggphs[i] = calloc(1, sizeof(*aggphs[i])); 
          margo_addr_dup(mid, svr_addr, &(aggphs[i]->addr));
          aggphs[i]->provider_id = p_id; 
          i++;
        }
        p->use_aggregator = 1;
        p->aggphs = aggphs;
    } else {
        fprintf(stderr, "AGGREGATOR_ADDRESS_FILE is not set. Continuing on without aggregator support");
    }
#endif

    if(a.push_finalize_callback)
        margo_provider_push_finalize_callback(mid, p, &collector_finalize_provider, p);

    if(provider)
        *provider = p;
    margo_info(mid, "COLLECTOR provider registration done");
    return COLLECTOR_SUCCESS;
}

static void collector_finalize_provider(void* p)
{
    collector_provider_t provider = (collector_provider_t)p;
    margo_info(provider->mid, "Finalizing COLLECTOR provider");
    margo_deregister(provider->mid, provider->metric_fetch_id);
    margo_deregister(provider->mid, provider->list_metrics_id);
    /* deregister other RPC ids ... */
    remove_all_metrics(provider);
#ifdef USE_AGGREGATOR
    //DEREGISTER_AGGREGATOR_CLIENT_AND_PROVIDER_HANDLES();
#endif
    free(provider);
    margo_info(provider->mid, "COLLECTOR provider successfuly finalized");
}

int collector_provider_destroy(
        collector_provider_t provider)
{
    margo_instance_id mid = provider->mid;
    margo_info(mid, "Destroying COLLECTOR provider");
    /* pop the finalize callback */
    margo_provider_pop_finalize_callback(provider->mid, provider);
    /* call the callback */
    collector_finalize_provider(provider);
    margo_info(mid, "COLLECTOR provider successfuly destroyed");
    return COLLECTOR_SUCCESS;
}

collector_return_t collector_provider_metric_create(const char *ns, const char *name, collector_metric_type_t t, const char *desc, collector_taglist_t tl, collector_metric_t* m, collector_provider_t provider)
{
    if(!ns || !name)
        return COLLECTOR_ERR_INVALID_NAME;

    /* create an id for the new metric */
    collector_metric_id_t id;
    collector_id_from_string_identifiers(ns, name, tl->taglist, tl->num_tags, &id);

    /* allocate a metric, set it up, and add it to the provider */
    collector_metric* metric = (collector_metric*)calloc(1, sizeof(*metric));
    ABT_mutex_create(&metric->metric_mutex);
    metric->id  = id;
    strcpy(metric->name, name);
    strcpy(metric->ns, ns);
    strcpy(metric->desc, desc);
    metric->type = t;
    metric->taglist = tl;
    metric->buffer_index = 0;
    metric->buffer = (collector_metric_buffer)calloc(METRIC_BUFFER_SIZE, sizeof(collector_metric_sample));
    add_metric(provider, metric);

    fprintf(stderr, "\nCreated metric %d of type %d\n", id, metric->type);
    fprintf(stderr, "Num metrics is: %lu\n", provider->num_metrics);

#ifdef USE_AGGREGATOR
    //aggregator_stream_id stream_id = aggregator_stream_create(string name, ...); //va_arg string list
    //aggregator_stream_attach_buffer((void*) buffer, func f_returning_current_buffer_index, uint8_t update_frequency_in_seconds);
#endif
    *m = metric;

    return COLLECTOR_SUCCESS;
}

static void collector_metric_fetch_ult(hg_handle_t h)
{
    hg_return_t hret;
    metric_fetch_in_t  in;
    metric_fetch_out_t out;
    hg_bulk_t local_bulk;

    /* find the margo instance */
    margo_instance_id mid = margo_hg_handle_get_instance(h);

    /* find the provider */
    const struct hg_info* info = margo_get_info(h);
    collector_provider_t provider = (collector_provider_t)margo_registered_data(mid, info->id);

    /* deserialize the input */
    hret = margo_get_input(h, &in);
    if(hret != HG_SUCCESS) {
        margo_info(provider->mid, "Could not deserialize output (mercury error %d)", hret);
        out.ret = COLLECTOR_ERR_FROM_MERCURY;
        goto finish;
    }

    /* create a bulk region */
    collector_metric_buffer b = calloc(in.count, sizeof(collector_metric_sample));
    hg_size_t buf_size = in.count * sizeof(collector_metric_sample);
    hret = margo_bulk_create(mid, 1, (void**)&b, &buf_size, HG_BULK_READ_ONLY, &local_bulk);

    if(hret != HG_SUCCESS) {
        margo_info(provider->mid, "Could not create bulk_handle (mercury error %d)", hret);
        out.ret = COLLECTOR_ERR_FROM_MERCURY;
        goto finish;
    }

    collector_metric_id_t requested_id = in.metric_id;
    collector_metric* metric = find_metric(provider, &(requested_id));
    if(!metric) {
        out.ret = COLLECTOR_ERR_INVALID_METRIC;
	goto finish;
    }


    out.name = (char*)malloc(36*sizeof(char));
    out.ns = (char*)malloc(36*sizeof(char));
    strcpy(out.name, metric->name);
    strcpy(out.ns, metric->ns);

    /* copyout metric buffer of requested size */
    if(metric->buffer_index < in.count) {
        out.actual_count = metric->buffer_index;
        memcpy(b, metric->buffer, out.actual_count*sizeof(collector_metric_sample));
    } else {
	out.actual_count = in.count;
        memcpy(b, metric->buffer + (metric->buffer_index - out.actual_count), out.actual_count*sizeof(collector_metric_sample));
    }

    /* do the bulk transfer */
    hret = margo_bulk_transfer(mid, HG_BULK_PUSH, info->addr, in.bulk, 0, local_bulk, 0, buf_size);
    if(hret != HG_SUCCESS) {
        margo_info(provider->mid, "Could not create bulk_handle (mercury error %d)", hret);
        out.ret = COLLECTOR_ERR_FROM_MERCURY;
        goto finish;
    }

    /* set the response */
    out.ret = COLLECTOR_SUCCESS;

finish:
    free(b);
    hret = margo_respond(h, &out);
    hret = margo_free_input(h, &in);
    margo_destroy(h);
    margo_bulk_free(local_bulk);
}
static DEFINE_MARGO_RPC_HANDLER(collector_metric_fetch_ult)

collector_return_t collector_provider_metric_destroy(collector_metric_t m, collector_provider_t provider)
{

    /* find the metric */
    collector_metric* metric = find_metric(provider, &m->id);
    if(!metric) {
        return COLLECTOR_ERR_INVALID_METRIC;
    }

#ifdef USE_AGGREGATOR
    //aggregator_stream_detach_buffer(aggregator_stream_id stream_id);
    //aggregator_stream_destroy(aggregator_stream_id stream_id);
#endif

    /* remove the metric from the provider */
    remove_metric(provider, &metric->id);

    ABT_mutex_free(&metric->metric_mutex);
    free(metric->buffer);

    return COLLECTOR_SUCCESS;
}

collector_return_t collector_provider_destroy_all_metrics(collector_provider_t provider)
{

    remove_all_metrics(provider);

    return COLLECTOR_SUCCESS;
}

static void collector_list_metrics_ult(hg_handle_t h)
{
    hg_return_t hret;
    list_metrics_in_t  in;
    list_metrics_out_t out;
    out.ids = NULL;

    /* find margo instance */
    margo_instance_id mid = margo_hg_handle_get_instance(h);

    /* find provider */
    const struct hg_info* info = margo_get_info(h);
    collector_provider_t provider = (collector_provider_t)margo_registered_data(mid, info->id);

    /* deserialize the input */
    hret = margo_get_input(h, &in);
    if(hret != HG_SUCCESS) {
        margo_error(mid, "Could not deserialize output (mercury error %d)", hret);
        out.ret = COLLECTOR_ERR_FROM_MERCURY;
        goto finish;
    }

    /* allocate array of metric ids */
    out.ret   = COLLECTOR_SUCCESS;
    out.count = provider->num_metrics < in.max_ids ? provider->num_metrics : in.max_ids;
    out.ids   = (collector_metric_id_t*)calloc(provider->num_metrics, sizeof(*out.ids));

    /* iterate over the hash of metrics to fill the array of metric ids */
    unsigned i = 0;
    collector_metric *r, *tmp;
    HASH_ITER(hh, provider->metrics, r, tmp) {
        out.ids[i++] = r->id;
    }

    margo_debug(mid, "Listed metrics");

finish:
    hret = margo_respond(h, &out);
    hret = margo_free_input(h, &in);
    free(out.ids);
    margo_destroy(h);
}
static DEFINE_MARGO_RPC_HANDLER(collector_list_metrics_ult)

collector_return_t collector_provider_register_backend()
{
    return COLLECTOR_SUCCESS;
}

static inline collector_metric* find_metric(
        collector_provider_t provider,
        const collector_metric_id_t* id)
{
    collector_metric* metric = NULL;

    HASH_FIND(hh, provider->metrics, id, sizeof(collector_metric_id_t), metric);
    return metric;
}

static inline collector_return_t add_metric(
        collector_provider_t provider,
        collector_metric* metric)
{

    collector_metric* existing = find_metric(provider, &(metric->id));
    if(existing) {
        return COLLECTOR_ERR_INVALID_METRIC;
    }
    HASH_ADD(hh, provider->metrics, id, sizeof(collector_metric_id_t), metric);
    provider->num_metrics += 1;

    return COLLECTOR_SUCCESS;
}

static inline collector_return_t remove_metric(
        collector_provider_t provider,
        const collector_metric_id_t* id)
{
    collector_metric* metric = find_metric(provider, id);
    if(!metric) {
        return COLLECTOR_ERR_INVALID_METRIC;
    }
    collector_return_t ret = COLLECTOR_SUCCESS;
    HASH_DEL(provider->metrics, metric);
    free(metric);
    provider->num_metrics -= 1;
    return ret;
}

static inline void remove_all_metrics(
        collector_provider_t provider)
{
    collector_metric *r, *tmp;
    HASH_ITER(hh, provider->metrics, r, tmp) {
        HASH_DEL(provider->metrics, r);
        free(r);
    }
    provider->num_metrics = 0;
}
