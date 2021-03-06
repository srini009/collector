/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include <margo.h>
#include <assert.h>
#include <collector/collector-client.h>
#include <collector/collector-metric.h>

#define FATAL(...) \
    do { \
        margo_critical(__VA_ARGS__); \
        exit(-1); \
    } while(0)

int main(int argc, char** argv)
{
    if(argc != 3) {
        fprintf(stderr,"Usage: %s <server address> <provider id>\n", argv[0]);
        exit(-1);
    }

    collector_return_t ret;
    hg_return_t hret;
    const char* svr_addr_str = argv[1];
    uint16_t    provider_id  = atoi(argv[2]);

    margo_instance_id mid = margo_init("na+sm://", MARGO_CLIENT_MODE, 0, 0);
    assert(mid);

    hg_addr_t svr_addr;
    hret = margo_addr_lookup(mid, svr_addr_str, &svr_addr);
    if(hret != HG_SUCCESS) {
        FATAL(mid,"margo_addr_lookup failed for address %s", svr_addr_str);
    }

    collector_client_t collector_clt;
    collector_metric_handle_t collector_rh;

    margo_info(mid, "Creating COLLECTOR client");
    ret = collector_client_init(mid, &collector_clt);
    if(ret != COLLECTOR_SUCCESS) {
        FATAL(mid,"collector_client_init failed (ret = %d)", ret);
    }

    size_t count = 5;
    collector_metric_id_t *ids;
    ids = (collector_metric_id_t *)malloc(count*sizeof(collector_metric_id_t));
    ret = collector_remote_list_metrics(collector_clt, svr_addr, provider_id, &ids, &count);

    collector_taglist_t taglist;
    collector_taglist_create(&taglist, 3, "tag1", "tag2", "tag3");

    collector_metric_id_t id;
    collector_remote_metric_get_id("srini", "testmetric2", taglist, &id);
    fprintf(stderr, "Retrieved metric id is: %d\n", id);

    if(ret != COLLECTOR_SUCCESS) {
	fprintf(stderr, "collector_remote_list_metrics failed (ret = %d)\n", ret);
    } else {
	fprintf(stderr, "Retrieved a total of %lu metrics\n", count);
        size_t j = 0;
        for(j = 0; j < count; j++)
           fprintf(stderr, "Retrieved metric with id: %d\n", ids[j]);
    }

    ret = collector_remote_metric_handle_create(
            collector_clt, svr_addr, provider_id,
            id, &collector_rh);

    if(ret != COLLECTOR_SUCCESS) {
        FATAL(mid,"collector_metric_handle_create failed (ret = %d)", ret);
    }

    int64_t num_samples_requested = 5;
    char *name, *ns;
    collector_metric_buffer buf;
    ret = collector_remote_metric_fetch(collector_rh, &num_samples_requested, &buf, &name, &ns);
    fprintf(stderr, "Number of metrics fetched %lu, with name %s and ns %s\n", num_samples_requested, name, ns);
    int i;
    collector_metric_sample *b = buf;
    for (i = 0; i < num_samples_requested; i++) {
        fprintf(stderr, "Values are : %f, and %f\n", b[i].val, b[i].time);
    }

    margo_info(mid, "Releasing metric handle");
    ret = collector_remote_metric_handle_release(collector_rh);
    if(ret != COLLECTOR_SUCCESS) {
        FATAL(mid,"collector_metric_handle_release failed (ret = %d)", ret);
    }

    margo_info(mid, "Finalizing client");
    ret = collector_client_finalize(collector_clt);
    if(ret != COLLECTOR_SUCCESS) {
        FATAL(mid,"collector_client_finalize failed (ret = %d)", ret);
    }

    hret = margo_addr_free(mid, svr_addr);
    if(hret != HG_SUCCESS) {
        FATAL(mid,"Could not free address (margo_addr_free returned %d)", hret);
    }

    margo_finalize(mid);

    return 0;
}
