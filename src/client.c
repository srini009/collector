/*
 * (C) 2020 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#include <stdarg.h>
#include "types.h"
#include "client.h"
#include "provider.h"
#include "collector/collector-client.h"
#include "collector/collector-common.h"

collector_return_t collector_client_init(margo_instance_id mid, collector_client_t* client)
{
    collector_client_t c = (collector_client_t)calloc(1, sizeof(*c));
    if(!c) return COLLECTOR_ERR_ALLOCATION;

    c->mid = mid;

    hg_bool_t flag;
    hg_id_t id;
    margo_registered_name(mid, "collector_remote_metric_fetch", &id, &flag);

    if(flag == HG_TRUE) {
        margo_registered_name(mid, "collector_remote_metric_fetch", &c->metric_fetch_id, &flag);
        margo_registered_name(mid, "collector_remote_list_metrics", &c->list_metrics_id, &flag);
    } else {
        c->metric_fetch_id = MARGO_REGISTER(mid, "collector_remote_metric_fetch", metric_fetch_in_t, metric_fetch_out_t, NULL);
        c->list_metrics_id = MARGO_REGISTER(mid, "collector_remote_list_metrics", list_metrics_in_t, list_metrics_out_t, NULL);
    }

    c->num_metric_handles = 0;
    *client = c;
    return COLLECTOR_SUCCESS;
}

collector_return_t collector_client_finalize(collector_client_t client)
{
    if(client->num_metric_handles != 0) {
        fprintf(stderr,  
                "Warning: %ld metric handles not released when collector_client_finalize was called\n",
                client->num_metric_handles);
    }
    free(client);
    return COLLECTOR_SUCCESS;
}

/* APIs for microservice clients */
collector_return_t collector_taglist_create(collector_taglist_t *taglist, int num_tags, ...) 
{
    *taglist = (collector_taglist_t)malloc(sizeof(collector_taglist));
    va_list valist;
    va_start(valist, num_tags);

    (*taglist)->taglist = (char **)malloc(num_tags*sizeof(char*));
    (*taglist)->num_tags = num_tags;
    int i = 0;

    for(i = 0; i < num_tags; i++) {
        (*taglist)->taglist[i] = (char*)malloc(36*sizeof(char*));
	strcpy((*taglist)->taglist[i], va_arg(valist, char*));
    }

    va_end(valist);
    return COLLECTOR_SUCCESS;

}

collector_return_t collector_taglist_destroy(collector_taglist_t taglist)
{
    int i;
    for(i = 0; i < taglist->num_tags; i++) {
        free(taglist->taglist[i]);
    }

    free(taglist->taglist);
    free(taglist);

    return COLLECTOR_SUCCESS;

}

collector_return_t collector_metric_create(const char *ns, const char *name, collector_metric_type_t t, const char *desc, collector_taglist_t taglist, collector_metric_t* m, collector_provider_t p)
{
    return collector_provider_metric_create(ns, name, t, desc, taglist, m, p);
}

collector_return_t collector_metric_destroy(collector_metric_t m, collector_provider_t p)
{
    return collector_provider_metric_destroy(m, p);
}

collector_return_t collector_metric_destroy_all(collector_provider_t p)
{
    return collector_provider_destroy_all_metrics(p);
}

collector_return_t collector_metric_update(collector_metric_t m, double val)
{
    switch(m->type) {
        case COLLECTOR_TYPE_COUNTER:
          if((m->buffer_index >=1) && (m->buffer[m->buffer_index-1].val > val)) 
              return COLLECTOR_ERR_INVALID_VALUE;
          break;
        case COLLECTOR_TYPE_TIMER:
          if(val < 0)
              return COLLECTOR_ERR_INVALID_VALUE;
          break;
        case COLLECTOR_TYPE_GAUGE:
          break;
    }

    ABT_unit_id self_id;
  
    ABT_mutex_lock(m->metric_mutex);
    ABT_self_get_thread_id(&self_id);
        
    m->buffer[m->buffer_index].val = val;
    m->buffer[m->buffer_index].time = ABT_get_wtime();
    m->buffer[m->buffer_index].sample_id = self_id;
    m->buffer_index++;

    ABT_mutex_unlock(m->metric_mutex);

    return COLLECTOR_SUCCESS;
}

collector_return_t collector_metric_update_gauge_by_fixed_amount(collector_metric_t m, double diff)
{
    switch(m->type) {
        case COLLECTOR_TYPE_COUNTER:
        case COLLECTOR_TYPE_TIMER:
             return COLLECTOR_ERR_INVALID_VALUE;
    }

    ABT_unit_id self_id;

    ABT_mutex_lock(m->metric_mutex);
    ABT_self_get_thread_id(&self_id);

    if(m->buffer_index) {
        m->buffer[m->buffer_index].val = m->buffer[m->buffer_index - 1].val + diff;
    } else {
        m->buffer[m->buffer_index].val = 1;
    }

    m->buffer[m->buffer_index].time = ABT_get_wtime();
    m->buffer[m->buffer_index].sample_id = self_id;
    m->buffer_index++;

unlock:
    ABT_mutex_unlock(m->metric_mutex);

    return COLLECTOR_SUCCESS;
}

collector_return_t collector_metric_register_retrieval_callback(char *ns, func f)
{
    fprintf(stderr, "Callback function for namespace: %s is not yet implmented\n", ns);

    return COLLECTOR_SUCCESS;
}

collector_return_t collector_metric_dump_histogram(collector_metric_t m, const char *filename, size_t num_buckets)
{
    double max = 0;
    double min = 9999999999999;

    fprintf(stderr, "Invoked dump histogram\n");
    int i = 0; 
    size_t *buckets = (size_t*)calloc(num_buckets, sizeof(size_t));
    for(i = 0 ; i < m->buffer_index; i++) {
        if(m->buffer[i].val > max)
            max = m->buffer[i].val;
	if(m->buffer[i].val < min)
	    min = m->buffer[i].val;
    }

    int bucket_index;
    for(i = 0 ; i < m->buffer_index; i++) {
        bucket_index = (int)(((m->buffer[i].val - min)/(max - min))*num_buckets);
        buckets[bucket_index]++;
    }

    FILE *fp = fopen(filename, "w");
    fprintf(fp, "%lu, %lf, %lf\n", num_buckets, min, max);
    for(i = 0; i < num_buckets; i++)
        fprintf(fp, "%lu\n", buckets[i]);
    fclose(fp);
    free(buckets);

}

collector_return_t collector_metric_dump_raw_data(collector_metric_t m, const char *filename)
{

    FILE *fp = fopen(filename, "w");
    int i;
    for(i = 0; i < m->buffer_index; i++)
        fprintf(fp, "%.9lf, %.9lf, %lu\n", m->buffer[i].val, m->buffer[i].time, m->buffer[i].sample_id);
    fclose(fp);
}

/* APIs for remote monitoring clients */

collector_return_t collector_remote_metric_get_id(char *ns, char *name, collector_taglist_t taglist, collector_metric_id_t* metric_id)
{
    if(!ns || !name)
        return COLLECTOR_ERR_INVALID_NAME;

    collector_id_from_string_identifiers(ns, name, taglist->taglist, taglist->num_tags, metric_id);

    return COLLECTOR_SUCCESS;
}

collector_return_t collector_remote_metric_fetch(collector_metric_handle_t handle, int64_t *num_samples_requested, collector_metric_buffer *buf, char **name, char **ns)
{
    hg_handle_t h;
    metric_fetch_in_t in;
    metric_fetch_out_t out;
    hg_bulk_t local_bulk;

    hg_return_t ret;
    in.metric_id = handle->metric_id;

    if(*num_samples_requested >= METRIC_BUFFER_SIZE || *num_samples_requested < 0)
        *num_samples_requested = METRIC_BUFFER_SIZE;

    in.count = *num_samples_requested;

    collector_metric_buffer b = (collector_metric_buffer)calloc(*num_samples_requested, sizeof(collector_metric_sample));
    hg_size_t segment_sizes[1] = {*num_samples_requested*sizeof(collector_metric_sample)};
    void *segment_ptrs[1] = {(void*)b};

    margo_bulk_create(handle->client->mid, 1,  segment_ptrs, segment_sizes, HG_BULK_WRITE_ONLY, &local_bulk);
    in.bulk = local_bulk;

    ret = margo_create(handle->client->mid, handle->addr, handle->client->metric_fetch_id, &h);
    if(ret != HG_SUCCESS)         
        return COLLECTOR_ERR_FROM_MERCURY; 

    ret = margo_provider_forward(handle->provider_id, h, &in);
    if(ret != HG_SUCCESS) {
        margo_destroy(h);
	return COLLECTOR_ERR_FROM_MERCURY;
    }

    ret = margo_get_output(h, &out);
    if(ret != HG_SUCCESS) {
	margo_free_output(h, &out);
        margo_destroy(h);
	return COLLECTOR_ERR_FROM_MERCURY;
    }

    *num_samples_requested = out.actual_count;
    *buf = b;
    *name = (char*)malloc(36*sizeof(char));
    *ns = (char*)malloc(36*sizeof(char));
    strcpy(*name, out.name);
    strcpy(*ns, out.ns);

    margo_free_output(h, &out);
    margo_destroy(h);
    margo_bulk_free(local_bulk);

    return out.ret;
}

collector_return_t collector_remote_metric_handle_create(
        collector_client_t client,
        hg_addr_t addr,
        uint16_t provider_id,
        collector_metric_id_t metric_id,
        collector_metric_handle_t* handle)
{
    if(client == COLLECTOR_CLIENT_NULL)
        return COLLECTOR_ERR_INVALID_ARGS;

    collector_metric_handle_t rh =
        (collector_metric_handle_t)calloc(1, sizeof(*rh));

    if(!rh) return COLLECTOR_ERR_ALLOCATION;

    hg_return_t ret = margo_addr_dup(client->mid, addr, &(rh->addr));
    if(ret != HG_SUCCESS) {
        free(rh);
        return COLLECTOR_ERR_FROM_MERCURY;
    }

    rh->client      = client;
    rh->provider_id = provider_id;
    rh->metric_id   = metric_id;
    rh->refcount    = 1;

    client->num_metric_handles += 1;

    *handle = rh;
    return COLLECTOR_SUCCESS;
}

collector_return_t collector_remote_metric_handle_ref_incr(
        collector_metric_handle_t handle)
{
    if(handle == COLLECTOR_METRIC_HANDLE_NULL)
        return COLLECTOR_ERR_INVALID_ARGS;
    handle->refcount += 1;
    return COLLECTOR_SUCCESS;
}

collector_return_t collector_remote_metric_handle_release(collector_metric_handle_t handle)
{
    if(handle == COLLECTOR_METRIC_HANDLE_NULL)
        return COLLECTOR_ERR_INVALID_ARGS;
    handle->refcount -= 1;
    if(handle->refcount == 0) {
        margo_addr_free(handle->client->mid, handle->addr);
        handle->client->num_metric_handles -= 1;
        free(handle);
    }
    return COLLECTOR_SUCCESS;
}

collector_return_t collector_remote_list_metrics(collector_client_t client, hg_addr_t addr, uint16_t provider_id, collector_metric_id_t** ids, size_t* count)
{
    hg_handle_t h;
    list_metrics_in_t  in;
    list_metrics_out_t out;
    collector_return_t ret;
    hg_return_t hret;

    in.max_ids = *count;

    hret = margo_create(client->mid, addr, client->list_metrics_id, &h);
    if(hret != HG_SUCCESS)
        return COLLECTOR_ERR_FROM_MERCURY;

    hret = margo_provider_forward(provider_id, h, &in);
    if(hret != HG_SUCCESS) {
        margo_destroy(h);
        return COLLECTOR_ERR_FROM_MERCURY;
    }

    hret = margo_get_output(h, &out);
    if(hret != HG_SUCCESS) {
        margo_destroy(h);
        return COLLECTOR_ERR_FROM_MERCURY;
    }

    ret = out.ret;
    if(ret == COLLECTOR_SUCCESS) {
        *count = out.count;
        memcpy(*ids, out.ids, out.count*sizeof(collector_metric_id_t));
    }
    
    margo_free_output(h, &out);
    margo_destroy(h);
    return ret;
}
