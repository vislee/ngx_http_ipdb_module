/*
 * Copyright (C) vislee
 */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "ipdb/ipdb.h"

typedef struct {
    ipdb_reader    *ipdb;
    ngx_str_t      *language;
} ngx_http_ipdb_conf_t;


typedef struct {
    ngx_str_t    *name;
    uintptr_t     data;
} ngx_http_ipdb_var_t;


static char *ngx_http_ipdb_language(ngx_conf_t *cf, void *post, void *data);
static ngx_int_t ngx_http_ipdb_add_variables(ngx_conf_t *cf);
static void *ngx_http_ipdb_create_conf(ngx_conf_t *cf);
static void ngx_http_ipdb_cleanup(void *data);
static char *ngx_http_ipdb_open(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_ipdb_region_name_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_ipdb_city_name_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);


static ngx_conf_post_handler_pt  ngx_http_ipdb_language_p =
    ngx_http_ipdb_language;


static ngx_command_t  ngx_http_ipdb_commands[] = {

    { ngx_string("ipdb"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE12,
      ngx_http_ipdb_open,
      NGX_HTTP_MAIN_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("ipdb_language"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_FLAG,
      ngx_conf_set_str_slot,
      NGX_HTTP_MAIN_CONF_OFFSET,
      offsetof(ngx_http_ipdb_conf_t, language),
      &ngx_http_ipdb_language_p },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_ipdb_module_ctx = {
    ngx_http_ipdb_add_variables,          /* preconfiguration */
    NULL,                                 /* postconfiguration */

    ngx_http_ipdb_create_conf,            /* create main configuration */
    ngx_http_ipdb_init_conf,              /* init main configuration */

    NULL,                                 /* create server configuration */
    NULL,                                 /* merge server configuration */

    NULL,                                 /* create location configuration */
    NULL                                  /* merge location configuration */
};


ngx_module_t  ngx_http_ipdb_module = {
    NGX_MODULE_V1,
    &ngx_http_ipdb_module_ctx,            /* module context */
    ngx_http_ipdb_commands,               /* module directives */
    NGX_HTTP_MODULE,                      /* module type */
    NULL,                                 /* init master */
    NULL,                                 /* init module */
    NULL,                                 /* init process */
    NULL,                                 /* init thread */
    NULL,                                 /* exit thread */
    NULL,                                 /* exit process */
    NULL,                                 /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_http_variable_t  ngx_http_ipdb_vars[] = {

    { ngx_string("ipdb_region_name"), NULL,
      ngx_http_ipdb_region_name_variable,
      0, 0, 0 },

    { ngx_string("ipdb_city_name"), NULL,
      ngx_http_ipdb_city_name_variable,
      offsetof(GeoIPRecord, city), 0, 0 },

      ngx_http_null_variable
};


static char *
ngx_http_ipdb_language(ngx_conf_t *cf, void *post, void *data)
{
    ngx_str_t  *language = data;

    u_char  *p, *new;

    if (ngx_strcmp(language->data, "EN") == 0
        || ngx_strcmp(language->data, "CN") == 0) {

        return NGX_CONF_OK;
    }


    return NGX_CONF_ERR;
}


static ngx_int_t
ngx_http_ipdb_add_variables(ngx_conf_t *cf)
{
    ngx_http_variable_t  *var, *v;

    for (v = ngx_http_ipdb_vars; v->name.len; v++) {
        var = ngx_http_add_variable(cf, &v->name, v->flags);
        if (var == NULL) {
            return NGX_ERROR;
        }

        var->get_handler = v->get_handler;
        var->data = v->data;
    }

    return NGX_OK;
}


static void *
ngx_http_ipdb_create_conf(ngx_conf_t *cf)
{
    ngx_pool_cleanup_t     *cln;
    ngx_http_ipdb_conf_t   *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_ipdb_conf_t));
    if (conf == NULL) {
        return NULL;
    }


    cln = ngx_pool_cleanup_add(cf->pool, 0);
    if (cln == NULL) {
        return NULL;
    }

    cln->handler = ngx_http_ipdb_cleanup;
    cln->data = conf;

    return conf;
}


static void
ngx_http_ipdb_cleanup(void *data)
{
    ngx_http_ipdb_conf_t  *icf = data;

    if (icf->ipdb) {
        ipdb_reader_free(&icf->ipdb);
    }
}


static char *
ngx_http_ipdb_open(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_int_t              err;
    ngx_http_ipdb_conf_t  *icf = conf;

    ngx_str_t  *value;

    if (icf->ipdb) {
        return "is duplicate";
    }

    value = cf->args->elts;

    err = ipdb_reader_new((char *) value[1].data, &icf->ipdb);

    if (err || icf->ipdb == NULL) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "ipdb_reader_new(\"%V\") failed", &value[1]);

        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}
