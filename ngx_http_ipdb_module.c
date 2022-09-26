/*
 * Copyright (C) vislee
 */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "ipdb/ipdb.h"

#define NGX_IPDB_country_name    0
#define NGX_IPDB_region_name     1
#define NGX_IPDB_city_name       2
#define NGX_IPDB_owner_domain    3
#define NGX_IPDB_isp_domain      4
#define NGX_IPDB_latitude        5
#define NGX_IPDB_longitude       6
#define NGX_IPDB_timezone        7

#define NGX_IPDB_utc_offset          8
#define NGX_IPDB_china_admin_code    9
#define NGX_IPDB_idd_code            10
#define NGX_IPDB_country_code        11
#define NGX_IPDB_continent_code      12
#define NGX_IPDB_idc                 13
#define NGX_IPDB_base_station        14
#define NGX_IPDB_country_code3       15
#define NGX_IPDB_european_union      16
#define NGX_IPDB_currency_code       17
#define NGX_IPDB_currency_name       18
#define NGX_IPDB_anycast             19

// all item
#define NGX_IPDB_raw             20

typedef struct {
    ipdb_reader    *ipdb;
    ngx_array_t    *proxies;    /* array of ngx_cidr_t */
    ngx_flag_t      proxy_recursive;
} ngx_http_ipdb_main_conf_t;

typedef struct ngx_http_ipdb_loc_conf_s {
    ngx_str_t                 lang;
    ngx_str_t                 spec_addr;
    ngx_http_complex_value_t  key;
} ngx_http_ipdb_loc_conf_t;


static char *ngx_http_ipdb_language(ngx_conf_t *cf, void *post, void *data);
static ngx_int_t ngx_http_ipdb_add_variables(ngx_conf_t *cf);
static void *ngx_http_ipdb_create_main_conf(ngx_conf_t *cf);
static char *ngx_http_ipdb_init_main_conf(ngx_conf_t *cf, void *conf);
static void *ngx_http_ipdb_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_ipdb_merge_loc_conf(ngx_conf_t *cf,
    void *parent, void *child);
static void ngx_http_ipdb_cleanup(void *data);
static char *ngx_http_ipdb_open(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_ipdb_proxy(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_ipdb_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);


static ngx_conf_post_handler_pt  ngx_http_ipdb_language_p =
    ngx_http_ipdb_language;


static ngx_command_t  ngx_http_ipdb_commands[] = {

    { ngx_string("ipdb"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
      ngx_http_ipdb_open,
      NGX_HTTP_MAIN_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("ipdb_language"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_ipdb_loc_conf_t, lang),
      &ngx_http_ipdb_language_p },

    { ngx_string("ipdb_proxy"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
      ngx_http_ipdb_proxy,
      NGX_HTTP_MAIN_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("ipdb_proxy_recursive"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_MAIN_CONF_OFFSET,
      offsetof(ngx_http_ipdb_main_conf_t, proxy_recursive),
      NULL },

    { ngx_string("ipdb_specifies_addr"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_ipdb_loc_conf_t, spec_addr),
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_ipdb_module_ctx = {
    ngx_http_ipdb_add_variables,          /* preconfiguration */
    NULL,                                 /* postconfiguration */

    ngx_http_ipdb_create_main_conf,       /* create main configuration */
    ngx_http_ipdb_init_main_conf,         /* init main configuration */

    NULL,                                 /* create server configuration */
    NULL,                                 /* merge server configuration */

    ngx_http_ipdb_create_loc_conf,        /* create location configuration */
    ngx_http_ipdb_merge_loc_conf          /* merge location configuration */
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

    { ngx_string("ipdb_country_name"), NULL,
      ngx_http_ipdb_variable,
      NGX_IPDB_country_name, 0, 0 },

    { ngx_string("ipdb_region_name"), NULL,
      ngx_http_ipdb_variable,
      NGX_IPDB_region_name, 0, 0 },

    { ngx_string("ipdb_city_name"), NULL,
      ngx_http_ipdb_variable,
      NGX_IPDB_city_name, 0, 0 },

    { ngx_string("ipdb_owner_domain"), NULL,
      ngx_http_ipdb_variable,
      NGX_IPDB_owner_domain, 0, 0 },

    { ngx_string("ipdb_isp_domain"), NULL,
      ngx_http_ipdb_variable,
      NGX_IPDB_isp_domain, 0, 0 },

    { ngx_string("ipdb_latitude"), NULL,
      ngx_http_ipdb_variable,
      NGX_IPDB_latitude, 0, 0 },

    { ngx_string("ipdb_longitude"), NULL,
      ngx_http_ipdb_variable,
      NGX_IPDB_longitude, 0, 0 },

    { ngx_string("ipdb_timezone"), NULL,
      ngx_http_ipdb_variable,
      NGX_IPDB_timezone, 0, 0 },

    { ngx_string("ipdb_utc_offset"), NULL,
      ngx_http_ipdb_variable,
      NGX_IPDB_utc_offset, 0, 0 },

    { ngx_string("ipdb_china_admin_code"), NULL,
      ngx_http_ipdb_variable,
      NGX_IPDB_china_admin_code, 0, 0 },

    { ngx_string("ipdb_idd_code"), NULL,
      ngx_http_ipdb_variable,
      NGX_IPDB_idd_code, 0, 0 },

    { ngx_string("ipdb_country_code"), NULL,
      ngx_http_ipdb_variable,
      NGX_IPDB_country_code, 0, 0 },

    { ngx_string("ipdb_continent_code"), NULL,
      ngx_http_ipdb_variable,
      NGX_IPDB_continent_code, 0, 0 },

    { ngx_string("ipdb_idc"), NULL,
      ngx_http_ipdb_variable,
      NGX_IPDB_idc, 0, 0 },

    { ngx_string("ipdb_base_station"), NULL,
      ngx_http_ipdb_variable,
      NGX_IPDB_base_station, 0, 0 },

    { ngx_string("ipdb_country_code3"), NULL,
      ngx_http_ipdb_variable,
      NGX_IPDB_country_code3, 0, 0 },

    { ngx_string("ipdb_european_union"), NULL,
      ngx_http_ipdb_variable,
      NGX_IPDB_european_union, 0, 0 },

    { ngx_string("ipdb_currency_code"), NULL,
      ngx_http_ipdb_variable,
      NGX_IPDB_currency_code, 0, 0 },

    { ngx_string("ipdb_currency_name"), NULL,
      ngx_http_ipdb_variable,
      NGX_IPDB_currency_name, 0, 0 },

    { ngx_string("ipdb_anycast"), NULL,
      ngx_http_ipdb_variable,
      NGX_IPDB_anycast, 0, 0 },

    { ngx_string("ipdb_raw"), NULL,
      ngx_http_ipdb_variable,
      NGX_IPDB_raw, 0, 0 },

#if (nginx_version >= 1013004)
      ngx_http_null_variable
#else
    { ngx_null_string, NULL, NULL, 0, 0, 0 }
#endif
};


static char *
ngx_http_ipdb_language(ngx_conf_t *cf, void *post, void *data)
{
    ngx_str_t  *lang = data;

    if (ngx_strcmp(lang->data, "EN") == 0
        || ngx_strcmp(lang->data, "CN") == 0) {

        return NGX_CONF_OK;
    }

    return NGX_CONF_ERROR;
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
ngx_http_ipdb_create_main_conf(ngx_conf_t *cf)
{
    ngx_pool_cleanup_t          *cln;
    ngx_http_ipdb_main_conf_t   *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_ipdb_main_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->proxy_recursive = NGX_CONF_UNSET;

    cln = ngx_pool_cleanup_add(cf->pool, 0);
    if (cln == NULL) {
        return NULL;
    }

    cln->handler = ngx_http_ipdb_cleanup;
    cln->data = conf;

    return conf;
}


static char *
ngx_http_ipdb_init_main_conf(ngx_conf_t *cf, void *conf)
{
    ngx_http_ipdb_main_conf_t  *imcf = conf;

    ngx_conf_init_value(imcf->proxy_recursive, 0);

    return NGX_CONF_OK;
}


static void *
ngx_http_ipdb_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_ipdb_loc_conf_t  *ilcf;

    ilcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_ipdb_loc_conf_t));
    if (ilcf == NULL) {
        return NULL;
    }

    return ilcf;
}


static char *
ngx_http_ipdb_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_compile_complex_value_t   ccv;

    ngx_http_ipdb_loc_conf_t *prev = parent;
    ngx_http_ipdb_loc_conf_t *conf = child;

    ngx_conf_merge_str_value(conf->lang, prev->lang, "EN");
    ngx_conf_merge_str_value(conf->spec_addr, prev->spec_addr, "");

    if (conf->spec_addr.len > 0) {
        ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

        ccv.cf = cf;
        ccv.value = &conf->spec_addr;
        ccv.complex_value = &conf->key;

        if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
            return NGX_CONF_ERROR;
        }
    }

    return NGX_CONF_OK;
}


static void
ngx_http_ipdb_cleanup(void *data)
{
    ngx_http_ipdb_main_conf_t  *imcf = data;

    if (imcf->ipdb) {
        ipdb_reader_free(&imcf->ipdb);
    }
}


static char *
ngx_http_ipdb_open(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_int_t                   err;
    ngx_http_ipdb_main_conf_t  *imcf = conf;

    ngx_str_t  *value;

    if (imcf->ipdb) {
        return "is duplicate";
    }

    value = cf->args->elts;

    err = ipdb_reader_new((char *) value[1].data, &imcf->ipdb);

    if (err || imcf->ipdb == NULL) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "ipdb_reader_new(\"%V\") failed", &value[1]);

        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}

static ngx_int_t
ngx_http_ipdb_cidr_value(ngx_conf_t *cf, ngx_str_t *net, ngx_cidr_t *cidr)
{
    ngx_int_t  rc;

    if (ngx_strcmp(net->data, "255.255.255.255") == 0) {
        cidr->family = AF_INET;
        cidr->u.in.addr = 0xffffffff;
        cidr->u.in.mask = 0xffffffff;

        return NGX_OK;
    }

    rc = ngx_ptocidr(net, cidr);

    if (rc == NGX_ERROR) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid network \"%V\"", net);
        return NGX_ERROR;
    }

    if (rc == NGX_DONE) {
        ngx_conf_log_error(NGX_LOG_WARN, cf, 0,
                           "low address bits of %V are meaningless", net);
    }

    return NGX_OK;
}

static char *
ngx_http_ipdb_proxy(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_ipdb_main_conf_t  *imcf = conf;

    ngx_str_t   *value;
    ngx_cidr_t  cidr, *c;

    value = cf->args->elts;

    if (ngx_http_ipdb_cidr_value(cf, &value[1], &cidr) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    if (imcf->proxies == NULL) {
        imcf->proxies = ngx_array_create(cf->pool, 4, sizeof(ngx_cidr_t));
        if (imcf->proxies == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    c = ngx_array_push(imcf->proxies);
    if (c == NULL) {
        return NGX_CONF_ERROR;
    }

    *c = cidr;

    return NGX_CONF_OK;
}


// "a\tb\t\tc"
// ""
static char *
ngx_http_ipdb_get_index_item(char *v, ngx_int_t idx)
{
    char *p, *q;
    ngx_int_t n = 0;

    if (v == NULL) return NULL;

    if (idx >= NGX_IPDB_raw) return v;

    p = v;
    q = p;

    while (*p) {

        if (*p == '\t') {

            if (idx == n) {
                *p = 0;
                return q;
            }

            q = p + 1;
            ++n;
        }

        ++p;
    }

    if (*p == 0 && idx == n) {
        return q;
    }

    return NULL;
}


static ngx_int_t
ngx_http_ipdb_item_by_addr(ipdb_reader *reader, ngx_addr_t *addr,
    const char *lang, char *body)
{
    int node = 0;
    ngx_int_t err, i;
    off_t off = -1;
    size_t p = 0, o = 0, s = 0, e = 0;
    size_t len = reader->meta->fields_length;
    const char *content;

    for (i = 0; i < reader->meta->language_length; ++i) {
        if (ngx_strcmp(lang, reader->meta->language[i].name) == 0) {
            off = reader->meta->language[i].offset;
            break;
        }
    }

    if (off == -1) {
        return ErrNoSupportLanguage;
    }

    if (addr->sockaddr->sa_family == AF_INET) {
        if (!ipdb_reader_is_ipv4_support(reader)) {
            return ErrNoSupportIPv4;
        }

        struct sockaddr_in  *sin;

        sin = (struct sockaddr_in *) addr->sockaddr;
        // sin->sin_addr.s_addr
        err = ipdb_search(reader, (const u_char *)&sin->sin_addr.s_addr, 32, &node);
        if (err != ErrNoErr) {
            return err;
        }
    }
#if (NGX_HAVE_INET6)
    else if (addr->sockaddr->sa_family == AF_INET6) {
        if (!ipdb_reader_is_ipv6_support(reader)) {
            return ErrNoSupportIPv6;
        }

        struct in6_addr  *inaddr6;
        inaddr6 = &((struct sockaddr_in6 *) addr->sockaddr)->sin6_addr;
        err = ipdb_search(reader, (const u_char *)&inaddr6->s6_addr, 128, &node);
        if (err != ErrNoErr) {
            return err;
        }
    }
#endif
    else {
        return ErrIPFormat;
    }

    err = ipdb_resolve(reader, node, &content);
    if (err) {
        return err;
    }

    while (*(content + p)) {
        if (*(content + p) == '\t') {
            ++o;
        }
        if ((!e) && o == off + len) {
            e = p;
        }
        ++p;
        if (off && (!s) && (off_t)o == off) {
            s = p;
        }
    }
    if (!e) e = p;
    if (off + len > o + 1) {
        err = ErrDatabaseError;
    } else {
        strncpy(body, content + s, e - s);
        body[e - s] = 0;
    }

    return err;
}

static ngx_int_t
ngx_http_ipdb_variable(ngx_http_request_t *r, ngx_http_variable_value_t *v,
    uintptr_t data)
{
    size_t                       len;
    char                        *p, *t;
    char                         body[512];
    ngx_int_t                    err;
    ngx_str_t                    spec_addr;
    ngx_addr_t                   addr;

#if (nginx_version >= 1023000)
    ngx_table_elt_t             *xfwd;
#else
    ngx_array_t                 *xfwd;
#endif

    ngx_http_ipdb_main_conf_t   *imcf;
    ngx_http_ipdb_loc_conf_t    *ilcf;

#if (NGX_DEBUG)
    ngx_str_t               debug;
#endif

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
        "ngx_http_ipdb_variable");

    imcf = ngx_http_get_module_main_conf(r, ngx_http_ipdb_module);

    if (imcf == NULL || imcf->ipdb == NULL) {
        goto not_found;
    }

    ilcf = ngx_http_get_module_loc_conf(r, ngx_http_ipdb_module);

    if (ilcf == NULL || ilcf->lang.data == NULL) {
        goto not_found;
    }

    if (ilcf->key.value.len > 0) {
        if (ngx_http_complex_value(r, &ilcf->key, &spec_addr) != NGX_OK) {
            goto not_found;
        }

        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
        "ngx_http_ipdb_variable, spec_addr: \"%V\"", &spec_addr);

        t = ngx_pcalloc(r->pool, spec_addr.len + 1);
        if (t == NULL) {
            goto not_found;
        }

        ngx_memcpy(t, spec_addr.data, spec_addr.len);
        t[spec_addr.len] = 0;

        err = ipdb_reader_find(imcf->ipdb, (const char*)t,
            (const char *)ilcf->lang.data, body);

        ngx_pfree(r->pool, t);

    } else {
        addr.sockaddr = r->connection->sockaddr;
        addr.socklen = r->connection->socklen;

#if (nginx_version >= 1023000)
        xfwd = r->headers_in.x_forwarded_for;
#else
        xfwd = &r->headers_in.x_forwarded_for;
#endif

        if (
#if (nginx_version >= 1023000)
            xfwd != NULL
#else
            xfwd->nelts > 0
#endif
             && imcf->proxies != NULL) {
            (void) ngx_http_get_forwarded_addr(r, &addr, xfwd, NULL,
                imcf->proxies, imcf->proxy_recursive);
        }

        err = ngx_http_ipdb_item_by_addr(imcf->ipdb, &addr,
            (const char *)ilcf->lang.data, body);
    }

    if (err) {
        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
        "ngx_http_ipdb_variable, ipdb find error: %d", err);

        goto not_found;
    }

#if (NGX_DEBUG)
    debug.len = ngx_strlen(body);
    debug.data = (u_char *)body;
    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
        "ngx_http_ipdb_variable, item: \"%V\"", &debug);
#endif

    p = ngx_http_ipdb_get_index_item(body, data);
    if (p == NULL) {
        goto not_found;
    }
    len = ngx_strlen(p);

#if (NGX_DEBUG)
    debug.len = len;
    debug.data = (u_char *)p;
    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
        "ngx_http_ipdb_variable, %l res: \"%V\"", data, &debug);
#endif

    v->data = ngx_pnalloc(r->pool, len);
    if (v->data == NULL) {
        return NGX_ERROR;
    }

    ngx_memcpy(v->data, p, len);

    v->len = len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;

not_found:

    v->not_found = 1;

    return NGX_OK;
}
