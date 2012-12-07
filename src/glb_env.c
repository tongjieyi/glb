/*
 * Copyright (C) 2008-2012 Codership Oy <info@codership.com>
 *
 * $Id$
 */

#include "glb_env.h"
#include "glb_limits.h"
#include "glb_socket.h"

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>

/* Environment variable names */
static const char env_vip[] = "GLB_VIP"; // address that should be balanced
static const char env_ctrl[] = "GLB_CONTROL"; // address to accept control connections
static const char env_targets[] = "GLB_TARGETS"; // balancing targets

// Defaults relevant to ENV
static const char env_ctrl_addr_default[] = "127.0.0.1";
static const char env_vip_addr_default[] = "127.0.0.1";

/* convert string into array of tokens */
static bool
env_parse_target_string (char* targets,
                         const char*** dst_list,
                         int* dst_num)
{
    assert (targets);

    *dst_list = NULL;
    *dst_num  = 0;

    if (!targets) return true;

    size_t const tlen = strlen(targets);
    if (!tlen) return true;

    const char** list = NULL;
    int num = 0;

    size_t i;
    for (i = 1; i <= tlen; i++) /* we can skip the first string char */
    {
        if (isspace(targets[i]) || ',' == targets[i]) targets[i] = '\0';
        if (targets[i] == '\0' && targets[i-1] != '\0') num++;/* end of token */
    }

    list = calloc (num, sizeof(const char*));
    if (!list) return true;

    list[0] = targets;
    num = 1;

    for (i = 1; i <= tlen; i++)
    {
        if (targets[i-1] == '\0' && targets[i] != '\0') /* beginning of token */
        {
            list[num] = &targets[i];
            num++;
        }
    }

    *dst_list = list;
    *dst_num  = num;

    return false;
}

glb_cnf_t*
glb_env_parse ()
{
    bool err;

    if (!getenv (env_vip))     return NULL;
    if (!getenv (env_targets)) return NULL;

    glb_cnf_t* ret = glb_cnf_init(); // initialize to defaults
    if (!ret) return NULL;

    char* const vip = strdup (getenv (env_vip));
    if (vip)
    {
        err = glb_parse_addr(&ret->inc_addr, vip, env_vip_addr_default);
        free (vip);
    }
    else err = true;

    if (err) goto failure;

    char* const targets = strdup (getenv (env_targets));
    if (targets)
    {
        const char** dst_list = NULL;
        int          dst_num = 0;
        uint16_t     vip_port = glb_socket_addr_get_port (&ret->inc_addr);

        if (!env_parse_target_string (targets, &dst_list, &dst_num))
        {
            assert(dst_list);
            assert(dst_num >= 0);

            glb_cnf_t* tmp = glb_parse_dst_list(dst_list, dst_num,vip_port,ret);

            if (tmp)
            {
                ret = tmp;
            }
            else err = true;

            free (dst_list);
        }
        else err = true;

        free (targets);
    }
    else err = true;

    if (!err) return ret;

failure:

    free (ret);
    return NULL;
}


