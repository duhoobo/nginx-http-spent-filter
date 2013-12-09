
/*
 * Copyright (C) 2013-2014 Baofeng Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


#define NGX_HTTP_SPENT_MODE_OFF 0
#define NGX_HTTP_SPENT_MODE_ACTIVE 1
#define NGX_HTTP_SPENT_MODE_PASSIVE 2

typedef struct {
    ngx_int_t       mode;

    ngx_str_t       prefix;

    ngx_str_t       header;
    ngx_str_t       header_lc;
    ngx_uint_t      header_hash;
} ngx_http_spent_loc_conf_t;


static ngx_int_t ngx_http_spent_header_filter(ngx_http_request_t *r);
static char * ngx_http_spent_set_mode(ngx_conf_t *cf, ngx_command_t *cmd, 
    void *conf);
static char * ngx_http_spent_set_header(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static void * ngx_http_spent_create_loc_conf(ngx_conf_t *cf);
static char * ngx_http_spent_merge_loc_conf(ngx_conf_t *cf, void *parent, 
    void *child);
static ngx_int_t ngx_http_spent_filter_init(ngx_conf_t *cf);


static ngx_command_t ngx_http_spent_filter_commands[] = {

    { ngx_string("spent"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_spent_set_mode,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("spent_header"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_spent_set_header,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_spent_loc_conf_t, header),
      NULL },

    { ngx_string("spent_prefix"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_spent_loc_conf_t, prefix),
      NULL },
      
    ngx_null_command
};


static ngx_http_module_t ngx_http_spent_filter_module_ctx = {
    NULL,                               /* preconfiguration */
    ngx_http_spent_filter_init,         /* postconfiguration */

    NULL,                               /* create main configuration */
    NULL,                               /* init main configuration */

    NULL,                               /* create server configuration */
    NULL,                               /* merge server configuration */

    ngx_http_spent_create_loc_conf,     /* create location configuration */
    ngx_http_spent_merge_loc_conf       /* merge location configuration */
};


ngx_module_t ngx_http_spent_filter_module = {
    NGX_MODULE_V1,
    &ngx_http_spent_filter_module_ctx,  /* module context */
    ngx_http_spent_filter_commands,     /* module directives */
    NGX_HTTP_MODULE,                    /* module type */
    NULL,                               /* init master */
    NULL,                               /* init module */
    NULL,                               /* init process */  
    NULL,                               /* init thread */
    NULL,                               /* exit thread */
    NULL,                               /* exit process */
    NULL,                               /* exit master */
    NGX_MODULE_V1_PADDING
};

static ngx_http_output_header_filter_pt ngx_http_next_header_filter;


/* TODO
 */


static ngx_int_t
ngx_http_spent_header_filter(ngx_http_request_t *r)
{
    ngx_int_t                   found;
    ngx_uint_t                  i;
    u_char                      *value;
    size_t                      value_len;
    ngx_time_t                  *tp;
    ngx_list_part_t             *part;
    ngx_table_elt_t             *headers;
    ngx_table_elt_t             *h;
    ngx_http_spent_loc_conf_t   *slcf;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http spent filter");

    slcf = ngx_http_get_module_loc_conf(r, ngx_http_spent_filter_module);

    if (r != r->main || slcf->mode == NGX_HTTP_SPENT_MODE_OFF) {
        return ngx_http_next_header_filter(r);
    }

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http spent filter header \"%V\" in mode %d", 
                   &slcf->header, slcf->mode);

    found = 0;
    part = &r->headers_out.headers.part;
    headers = part->elts;

    for (i = 0; /* void */; i++) {

        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }

            part = part->next;
            headers = part->elts;
            i = 0;
        }

        if (headers[i].hash != slcf->header_hash 
            || headers[i].key.len != slcf->header.len
            || headers[i].lowcase_key == NULL) 
        {
            continue;
        }

        if (ngx_strncmp(headers[i].lowcase_key, slcf->header_lc.data, 
                        slcf->header_lc.len) != 0) 
        {
            continue;
        }

        found = 1;

        break;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http spent filter found: %d", found);

    if (!found && slcf->mode == NGX_HTTP_SPENT_MODE_PASSIVE) {
        return ngx_http_next_header_filter(r);
    }

    tp = ngx_timeofday();

    if (found) {

        value_len = slcf->prefix.len + sizeof("000.000;") - 1 
                    + headers[i].value.len + sizeof(";") - 1 
                    + slcf->prefix.len + sizeof("000.000") - 1 + 1;

        value = ngx_pcalloc(r->pool, value_len);
        if (value == NULL) {
            return NGX_ERROR;
        }

        ngx_snprintf(value, value_len, "%V%03d.%03d;%V;%V%03d.%03d",
                     &slcf->prefix, r->start_sec % 1000, r->start_msec,
                     &headers[i].value, 
                     &slcf->prefix, tp->sec % 1000, tp->msec);

        headers[i].value.data = value;
        headers[i].value.len = value_len - 1;

    } else {

        value_len = slcf->prefix.len + sizeof("000.000;") - 1
                    + slcf->prefix.len + sizeof("000.000") - 1 + 1;
        
        value = ngx_pcalloc(r->pool, value_len);
        if (value == NULL) {
            return NGX_ERROR;
        }

        ngx_snprintf(value, value_len, "%V%03d.%03d;%V%03d.%03d",
                     &slcf->prefix, r->start_sec % 1000, r->start_msec,
                     &slcf->prefix, tp->sec % 1000, tp->msec);

        h = ngx_list_push(&r->headers_out.headers);
        if (h == NULL) {
            return NGX_ERROR;
        }

        h->hash = slcf->header_hash;
        h->key.data = slcf->header.data;
        h->key.len = slcf->header.len;
        h->value.data = value;
        h->value.len = value_len - 1;
        h->lowcase_key = slcf->header_lc.data;
    }


    return ngx_http_next_header_filter(r);
}


static char *
ngx_http_spent_set_mode(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_spent_loc_conf_t *slcf = conf;

    ngx_str_t                   *value;

    value = cf->args->elts;

    if (ngx_strcmp(value[1].data, "off") == 0) {
        slcf->mode = NGX_HTTP_SPENT_MODE_OFF;

    } else if (ngx_strcmp(value[1].data, "active") == 0) {
        slcf->mode = NGX_HTTP_SPENT_MODE_ACTIVE;

    } else if (ngx_strcmp(value[1].data, "passive") == 0) {
        slcf->mode = NGX_HTTP_SPENT_MODE_PASSIVE;

    } else {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid parameter \"%V\" for directive \"spent\"",
                           &value[1]);
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}


static char *
ngx_http_spent_set_header(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    char *rv;
    ngx_http_spent_loc_conf_t *slcf = conf;

    if ((rv = ngx_conf_set_str_slot(cf, cmd, conf)) != NGX_CONF_OK) {
        return rv;
    }

    slcf->header_lc.len = slcf->header.len;
    slcf->header_lc.data = ngx_pcalloc(cf->pool, slcf->header_lc.len + 1);
    if (slcf->header_lc.data == NULL) {
        return "alloc for header_lc error";
    }

    slcf->header_hash = ngx_hash_strlow(slcf->header_lc.data, slcf->header.data,
                                        slcf->header.len);

    return NGX_CONF_OK;
}


static void *
ngx_http_spent_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_spent_loc_conf_t *slcf;

    slcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_spent_loc_conf_t));
    if (slcf == NULL) {
        return NULL;
    }

    slcf->mode = NGX_CONF_UNSET;
    slcf->header_hash = NGX_CONF_UNSET_UINT;

    return slcf;    
}


static char *
ngx_http_spent_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_spent_loc_conf_t *prev = parent;
    ngx_http_spent_loc_conf_t *conf = child;

    ngx_conf_merge_value(conf->mode, prev->mode, NGX_HTTP_SPENT_MODE_OFF);

    ngx_conf_merge_str_value(conf->header, prev->header, "X-Spent");
    ngx_conf_merge_str_value(conf->header_lc, prev->header_lc, "x-spent");
    ngx_conf_merge_uint_value(conf->header_hash, prev->header_hash,
                              ngx_hash_key_lc((u_char *) "X-Spent", 
                                               sizeof("X-Spent") - 1));

    ngx_conf_merge_str_value(conf->prefix, prev->prefix, "p");

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_spent_filter_init(ngx_conf_t *cf)
{
    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = ngx_http_spent_header_filter;

    return NGX_OK;
}

