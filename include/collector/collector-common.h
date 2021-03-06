/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __COLLECTOR_COMMON_H
#define __COLLECTOR_COMMON_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Error codes that can be returned by COLLECTOR functions.
 */
typedef enum collector_return_t {
    COLLECTOR_SUCCESS,
    COLLECTOR_ERR_INVALID_NAME,      /* Metric creation error - name or ns missing */
    COLLECTOR_ERR_ALLOCATION,        /* Allocation error */
    COLLECTOR_ERR_INVALID_ARGS,      /* Invalid argument */
    COLLECTOR_ERR_INVALID_PROVIDER,  /* Invalid provider id */
    COLLECTOR_ERR_INVALID_METRIC,    /* Invalid metric id */
    COLLECTOR_ERR_INVALID_VALUE,     /* Invalid metric update value */
    COLLECTOR_ERR_INVALID_BACKEND,   /* Invalid backend type */
    COLLECTOR_ERR_INVALID_CONFIG,    /* Invalid configuration */
    COLLECTOR_ERR_INVALID_TOKEN,     /* Invalid token */
    COLLECTOR_ERR_FROM_MERCURY,      /* Mercury error */
    COLLECTOR_ERR_FROM_ARGOBOTS,     /* Argobots error */
    COLLECTOR_ERR_OP_UNSUPPORTED,    /* Unsupported operation */
    COLLECTOR_ERR_OP_FORBIDDEN,      /* Forbidden operation */
    /* ... TODO add more error codes here if needed */
    COLLECTOR_ERR_OTHER              /* Other error */
} collector_return_t;

#define METRIC_BUFFER_SIZE 160000000

/**
 * @brief Identifier for a metric.
 */
typedef uint32_t collector_metric_id_t;

typedef enum collector_metric_type {
   COLLECTOR_TYPE_COUNTER,
   COLLECTOR_TYPE_TIMER,
   COLLECTOR_TYPE_GAUGE    
} collector_metric_type_t;

typedef struct collector_metric_sample {
   double time;
   double val;
   uint64_t sample_id;
} collector_metric_sample;

typedef collector_metric_sample* collector_metric_buffer;

typedef struct collector_taglist {
    char **taglist;
    int num_tags;
} collector_taglist;

typedef collector_taglist* collector_taglist_t;

inline uint32_t collector_hash(char *str);

inline void collector_id_from_string_identifiers(char *ns, char *name, char **taglist, int num_tags, collector_metric_id_t *id_);

/* djb2 hash from Dan Bernstein */
inline uint32_t
collector_hash(char *str)
{
    uint32_t hash = 5381;
    uint32_t c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }

    return hash;
}


inline void collector_id_from_string_identifiers(char *ns, char *name, char **taglist, int num_tags, collector_metric_id_t *id_)
{
    collector_metric_id_t id, temp_id;

    id = collector_hash(ns);
    temp_id = collector_hash(name);
    id = id^temp_id;

    /* XOR all the tag ids, so that any ordering of tags returns the same final metric id */
    int i;
    for(i = 0; i < num_tags; i++) {
	temp_id = collector_hash(taglist[i]);
	id = id^temp_id;
    }
   
    *id_ = id;
}

#ifdef __cplusplus
}
#endif

#endif
