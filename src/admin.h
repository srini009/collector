/*
 * (C) 2020 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef _ADMIN_H
#define _ADMIN_H

#include "types.h"
#include "collector/collector-admin.h"

typedef struct collector_admin {
   margo_instance_id mid;
} collector_admin;

#endif
