/*
 * Copyright (C)  Igor Sysoev; kevin_zhong
 *      mail: qq2000zhong@gmail.com
 *      date: 2011-11-21
 *      desc: to understand what those codes mean, please read the fucking source code first,
 *               if 7 days later, you dont understand yet.... mail to me !
 */

extern "C" {
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "comm/ngx_chain_util.h"
#include "comm/ngx_http_upstream_ext.h"
}

#include "comm/ngx_thrift_transport.h"
#include "comm/ngx_xdrive_datagram.h"
#include "comm/ngx_http_xdrive_parse.h"
#include "protocal/http_types.h"

using namespace xdrive::msg::http;
using namespace xdrive;

typedef struct
{
        ngx_http_upstream_conf_t upstream;
        size_t                   download_range_size;
        size_t                   download_speed_rate;
        ngx_msec_t               download_rate_check_interval;
}
ngx_http_xdrive_download_loc_conf_t;


enum  NGX_XDRIVE_PROXY_DECODE_STATUS {
        NGX_XDRIVE_PROXY_INIT,
        NGX_XDRIVE_PROXY_WAIT_HTTP_HEAD_HEAD,
        NGX_XDRIVE_PROXY_WAIT_HTTP_HEAD_BODY,
        NGX_XDRIVE_PROXY_WAIT_HTTP_BODY_HEAD,
        NGX_XDRIVE_PROXY_WAIT_HTTP_BODY_BODY,
};

typedef struct
{
        ngx_http_request_t *request;

        HttpHeadReq *       http_head_req;

        //for range request
        int64_t             total_size;
        int64_t             request_range_begin;
        int64_t             request_range_end;
        int64_t             complete_range_end;

        int64_t             rest_range_length;

        //for http head resp
        ngx_buf_t           head_buf;

        //for datagram decode
        int                 rest_datagram_length;
        ngx_uint_t          last_datagram_chunk : 1;

        ngx_uint_t          decode_status : 3;
        ngx_uint_t          request_range_set : 1;

        ngx_int_t (*proccess_http_head_resp)(ngx_http_request_t *r, HttpHeadResp *resp);
}
ngx_http_xdrive_download_ctx_t;


static ngx_str_t ngx_http_xdrive_download_hide_headers[] = {
        ngx_string("Status"),
        ngx_string("Content-Length"),
        ngx_string("Content-Range"),
        ngx_string("Set-Cookie"),
        ngx_null_string
};


static ngx_int_t ngx_http_xdrive_download_create_request(ngx_http_request_t *r);
static ngx_int_t ngx_http_xdrive_download_reinit_request(ngx_http_request_t *r);
static ngx_int_t ngx_http_xdrive_download_process_header(ngx_http_request_t *r);
static ngx_int_t ngx_http_xdrive_download_filter_init(void *data);
static ngx_int_t ngx_http_xdrive_download_filter(void *data, ssize_t bytes);
static void ngx_http_xdrive_download_abort_request(ngx_http_request_t *r);
static void ngx_http_xdrive_download_finalize_request(ngx_http_request_t *r, ngx_int_t download);

static void *ngx_http_xdrive_download_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_xdrive_download_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);

static char *ngx_http_xdrive_download_pass(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static ngx_int_t  ngx_http_proccess_first_head_resp(ngx_http_request_t *r, HttpHeadResp *resp);
static ngx_int_t  ngx_http_proccess_post_head_resp(ngx_http_request_t *r, HttpHeadResp *resp);

static ngx_int_t  ngx_http_xdrive_next_range(ngx_http_xdrive_download_ctx_t *ctx);

static void ngx_http_xdrive_check_dl_rate_handler(ngx_event_t *ev);

static void ngx_http_upstream_finalize_sub_request(ngx_http_request_t *r, ngx_http_upstream_t *u, ngx_int_t rc);

static ngx_conf_bitmask_t ngx_http_xdrive_download_next_upstream_masks[] = {
        { ngx_string("error"),          NGX_HTTP_UPSTREAM_FT_ERROR          },
        { ngx_string("timeout"),        NGX_HTTP_UPSTREAM_FT_TIMEOUT        },
        { ngx_string("invalid_header"), NGX_HTTP_UPSTREAM_FT_INVALID_HEADER },
        { ngx_string("not_found"),      NGX_HTTP_UPSTREAM_FT_HTTP_404       },
        { ngx_string("off"),            NGX_HTTP_UPSTREAM_FT_OFF            },
        { ngx_null_string,              0                                   }
};


static ngx_command_t ngx_http_xdrive_download_commands[] = {
        { ngx_string("xdrive_download_pass"),
          NGX_HTTP_LOC_CONF | NGX_HTTP_LIF_CONF | NGX_CONF_TAKE1,
          ngx_http_xdrive_download_pass,
          NGX_HTTP_LOC_CONF_OFFSET,
          0,
          NULL },

        { ngx_string("xdrive_download_range_size"),
          NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
          ngx_conf_set_size_slot,
          NGX_HTTP_LOC_CONF_OFFSET,
          offsetof(ngx_http_xdrive_download_loc_conf_t, download_range_size),
          NULL },

        { ngx_string("xdrive_download_speed_rate"),
          NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
          ngx_conf_set_size_slot,
          NGX_HTTP_LOC_CONF_OFFSET,
          offsetof(ngx_http_xdrive_download_loc_conf_t, download_speed_rate),
          NULL },        

        { ngx_string("xdrive_download_rate_check_interval"),
          NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
          ngx_conf_set_msec_slot,
          NGX_HTTP_LOC_CONF_OFFSET,
          offsetof(ngx_http_xdrive_download_loc_conf_t, download_rate_check_interval),
          NULL },

        { ngx_string("xdrive_download_connect_timeout"),
          NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
          ngx_conf_set_msec_slot,
          NGX_HTTP_LOC_CONF_OFFSET,
          offsetof(ngx_http_xdrive_download_loc_conf_t, upstream.connect_timeout),
          NULL },

        { ngx_string("xdrive_download_send_timeout"),
          NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
          ngx_conf_set_msec_slot,
          NGX_HTTP_LOC_CONF_OFFSET,
          offsetof(ngx_http_xdrive_download_loc_conf_t, upstream.send_timeout),
          NULL },

        { ngx_string("xdrive_download_buffer_size"),
          NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
          ngx_conf_set_size_slot,
          NGX_HTTP_LOC_CONF_OFFSET,
          offsetof(ngx_http_xdrive_download_loc_conf_t, upstream.buffer_size),
          NULL },

        { ngx_string("xdrive_download_read_timeout"),
          NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
          ngx_conf_set_msec_slot,
          NGX_HTTP_LOC_CONF_OFFSET,
          offsetof(ngx_http_xdrive_download_loc_conf_t, upstream.read_timeout),
          NULL },

        { ngx_string("xdrive_download_next_upstream"),
          NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_1MORE,
          ngx_conf_set_bitmask_slot,
          NGX_HTTP_LOC_CONF_OFFSET,
          offsetof(ngx_http_xdrive_download_loc_conf_t, upstream.next_upstream),
          &ngx_http_xdrive_download_next_upstream_masks },

        ngx_null_command
};


static ngx_http_module_t ngx_http_xdrive_download_module_ctx = {
        NULL,                                     /* preconfiguration */
        NULL,                                     /* postconfiguration */

        NULL,                                     /* create main configuration */
        NULL,                                     /* init main configuration */

        NULL,                                     /* create server configuration */
        NULL,                                     /* merge server configuration */

        ngx_http_xdrive_download_create_loc_conf, /* create location configration */
        ngx_http_xdrive_download_merge_loc_conf   /* merge location configration */
};


ngx_module_t ngx_http_xdrive_download_module = {
        NGX_MODULE_V1,
        &ngx_http_xdrive_download_module_ctx, /* module context */
        ngx_http_xdrive_download_commands,    /* module directives */
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

#if 0
#define  THRIFT_FILED_BEGIN_LEN 3
#define  THRIFT_FILED_STR_LEN_LEN 4
#define  THRIFT_FILED_STOP_LEN_LEN 1
#else
#define  THRIFT_FILED_BEGIN_LEN 0
#define  THRIFT_FILED_STR_LEN_LEN 0
#define  THRIFT_FILED_STOP_LEN_LEN 0
#endif

#define  NGX_XDRIVE_DATAGRAM_HEADER_EXT (NGX_XDRIVE_DATAGRAM_HEADER \
                + THRIFT_FILED_BEGIN_LEN + 1 + THRIFT_FILED_BEGIN_LEN + THRIFT_FILED_STR_LEN_LEN)


static ngx_int_t
ngx_http_xdrive_download_handler(ngx_http_request_t *r)
{
        ngx_int_t download;
        ngx_http_upstream_t *u;
        ngx_http_xdrive_download_ctx_t *ctx;
        ngx_http_xdrive_download_loc_conf_t *mlcf;

        if (!(r->method & (NGX_HTTP_GET | NGX_HTTP_HEAD)))
        {
                return NGX_HTTP_NOT_ALLOWED;
        }

        download = ngx_http_discard_request_body(r);

        if (download != NGX_OK)
        {
                return download;
        }

        if (ngx_http_set_content_type(r) != NGX_OK)
        {
                return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        if (ngx_http_upstream_create(r) != NGX_OK)
        {
                return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        u = r->upstream;

        ngx_str_set(&u->schema, "xdrive_download://"); //schema，没发现有什么用，打log貌似会有点用

        u->output.tag = (ngx_buf_tag_t)&ngx_http_xdrive_download_module;

        mlcf = (ngx_http_xdrive_download_loc_conf_t *)ngx_http_get_module_loc_conf(r, ngx_http_xdrive_download_module);
        u->conf = &mlcf->upstream;

        u->create_request = ngx_http_xdrive_download_create_request;
        u->reinit_request = ngx_http_xdrive_download_reinit_request;
        u->process_header = ngx_http_xdrive_download_process_header;
        u->abort_request = ngx_http_xdrive_download_abort_request;
        u->finalize_request = ngx_http_xdrive_download_finalize_request;

        ctx = (ngx_http_xdrive_download_ctx_t *)ngx_palloc(r->pool, sizeof(ngx_http_xdrive_download_ctx_t));
        if (ctx == NULL)
        {
                return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }
        ngx_memzero(ctx, sizeof(ngx_http_xdrive_download_ctx_t));

        ctx->request = r;

        ngx_http_set_ctx(r, ctx, ngx_http_xdrive_download_module);

        u->input_filter_init = ngx_http_xdrive_download_filter_init;

        u->input_filter = ngx_http_xdrive_download_filter;
        u->input_filter_ctx = ctx;

        u->buffering = 0;

        r->main->count++;

        ngx_http_upstream_init(r);

        return NGX_DONE;
}

static int64_t  ngx_to_range_end(int64_t start, int64_t chunk_size)
{
        return start + chunk_size;
}

static ngx_int_t
ngx_http_xdrive_download_create_request(ngx_http_request_t *r)
{
        size_t len;
        ngx_buf_t *b;
        ngx_chain_t *cl;
        ngx_http_xdrive_download_ctx_t *ctx;
        ngx_http_variable_value_t *vv;
        ngx_http_xdrive_download_loc_conf_t *mlcf;

        mlcf = (ngx_http_xdrive_download_loc_conf_t *)ngx_http_get_module_loc_conf(r,
                                                                                   ngx_http_xdrive_download_module);
        ctx = (ngx_http_xdrive_download_ctx_t *)ngx_http_get_module_ctx(r,
                                                                        ngx_http_xdrive_download_module);

        ctx->http_head_req = (HttpHeadReq *)ngx_palloc(r->pool, sizeof(HttpHeadReq));
        if (ctx->http_head_req == NULL)
                return NGX_ERROR;

        ::new (ctx->http_head_req)HttpHeadReq;

        ctx->http_head_req->method = HttpMethod::type(r->method);
        ctx->http_head_req->uri.assign((const char *)r->uri.data, r->uri.len);
        ctx->http_head_req->last = 1;

        ngx_http_parse_args(r, ctx->http_head_req->arguments);
        ngx_http_parse_cookies(r, ctx->http_head_req->cookies);

        //set range
        HttpBytesRange& bytes_range = ctx->http_head_req->bytes_range;
        if (ngx_http_parse_range(r, 0, bytes_range) != NGX_OK)
                return NGX_ERROR;

        if (!bytes_range.range_set)
        {
                bytes_range.range_set = 1;
                bytes_range.head = 0;
        }
        else {
                ctx->request_range_set = 1;
        }
        assert(bytes_range.head >= 0);

        ctx->request_range_begin = bytes_range.head;
        ctx->complete_range_end = bytes_range.head;
        ctx->request_range_end = bytes_range.last;

        if (bytes_range.last == NGX_CONF_UNSET)
        {
                bytes_range.last = ngx_to_range_end(bytes_range.head, mlcf->download_range_size);
        }
        else {
                assert(bytes_range.head < bytes_range.last);

                bytes_range.last = ngx_min(bytes_range.last,
                                           ngx_to_range_end(bytes_range.head, mlcf->download_range_size));
        }

        ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0, "range [%O-%O]",
                      bytes_range.head, bytes_range.last);

        static uint32_t seq = ngx_time();

        cl = ngx_datagram_encode(r->pool, r->connection->log, mlcf->upstream.buffer_size,
                                 ctx->http_head_req, ++seq, 0xf01);

        ctx->decode_status = NGX_XDRIVE_PROXY_WAIT_HTTP_HEAD_HEAD;
        ctx->proccess_http_head_resp = ngx_http_proccess_first_head_resp;

        r->upstream->request_bufs = cl;
        return NGX_OK;
}


static ngx_int_t
ngx_http_xdrive_download_reinit_request(ngx_http_request_t *r)
{
        return NGX_OK;
}


static ngx_int_t
ngx_http_xdrive_download_process_header_buf(ngx_http_request_t *r
                                            , ngx_buf_t *buf)
{
        size_t rest_len;
        ngx_http_xdrive_download_ctx_t *ctx;

        rest_len = buf->last - buf->pos;

        ctx = (ngx_http_xdrive_download_ctx_t *)ngx_http_get_module_ctx(r,
                                                                        ngx_http_xdrive_download_module);

        if (ctx->decode_status == NGX_XDRIVE_PROXY_WAIT_HTTP_HEAD_HEAD)
        {
                if (rest_len < NGX_XDRIVE_DATAGRAM_HEADER)
                        return NGX_AGAIN;

                ngx_xdrive_datagram_header_t header;
                if (ngx_decode_header(buf->pos, NGX_XDRIVE_DATAGRAM_HEADER,
                                      &header, r->connection->log) != NGX_OK)
                {
                        return NGX_HTTP_UPSTREAM_INVALID_HEADER;
                }

                if (header._type != 0x08f01)
                {
                        ngx_log_error(NGX_LOG_WARN, r->connection->log, 0,
                                      "xdrive_download ret type not legal = %d", header._type);

                        return NGX_HTTP_UPSTREAM_INVALID_HEADER;
                }

                if (header._status != 0)
                {
                        ngx_log_error(NGX_LOG_WARN, r->connection->log, 0,
                                      "xdrive_download ret status not ok in response = %d", header._status);

                        return NGX_HTTP_UPSTREAM_INVALID_HEADER;
                }

                rest_len -= NGX_XDRIVE_DATAGRAM_HEADER;
                buf->pos += NGX_XDRIVE_DATAGRAM_HEADER;
                ctx->rest_datagram_length = header._length - NGX_XDRIVE_DATAGRAM_HEADER;

                if (ctx->rest_datagram_length == 0)
                {
                        return NGX_HTTP_UPSTREAM_INVALID_HEADER;
                }

                if (ctx->rest_datagram_length > buf->end - buf->pos)
                {
                        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                                      "upstream sent too big header, head body-len=%d", ctx->rest_datagram_length);
                        return NGX_HTTP_UPSTREAM_INVALID_HEADER;
                }

                ctx->decode_status = NGX_XDRIVE_PROXY_WAIT_HTTP_HEAD_BODY;
                //go on
        }

        if (ctx->decode_status == NGX_XDRIVE_PROXY_WAIT_HTTP_HEAD_BODY)
        {
                if (rest_len < ctx->rest_datagram_length)
                        return NGX_AGAIN;

                ngx_chain_t cl;
                cl.buf = buf;
                cl.next = NULL;

                HttpHeadResp http_resp;
                if (ngx_datagram_decode_body(&cl, r->connection->log, &http_resp) != NGX_OK)
                {
                        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                                      "xdrive_download HttpHeadResp decode failed");

                        return NGX_ERROR;
                }

                ctx->last_datagram_chunk = http_resp.last;

                return ctx->proccess_http_head_resp(r, &http_resp);
        }
        else
                assert(0);
}


static ngx_int_t
ngx_http_proccess_first_head_resp(ngx_http_request_t *r, HttpHeadResp *resp)
{
        ngx_http_upstream_t *u;
        ngx_http_xdrive_download_ctx_t *ctx;
        ngx_http_xdrive_download_loc_conf_t *mlcf;
        HttpHeadResp& http_resp = *resp;
        HttpBytesRange *bytes_range;

        u = r->upstream;
        ctx = (ngx_http_xdrive_download_ctx_t *)ngx_http_get_module_ctx(r,
                                                                        ngx_http_xdrive_download_module);
        mlcf = (ngx_http_xdrive_download_loc_conf_t *)
               ngx_http_get_module_loc_conf(ctx->request, ngx_http_xdrive_download_module);

        u->headers_in.status_n = http_resp.http_status;
        u->state->status = http_resp.http_status;

        bytes_range = &http_resp.bytes_range;

        if (bytes_range->range_set)
        {
                if (bytes_range->head < 0 || bytes_range->last < 0
                    || bytes_range->head > bytes_range->last
                    || bytes_range->total_size <= 0
                    || bytes_range->last > bytes_range->total_size     //sick
                    || bytes_range->head != ctx->complete_range_end)
                {
                        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                                      "illegal resp range, %O->%O/%O, req start=%O",
                                      bytes_range->head, bytes_range->last, bytes_range->total_size,
                                      ctx->complete_range_end);

                        return NGX_HTTP_UPSTREAM_INVALID_HEADER;
                }

                ctx->request_range_set = 1;

                //adjust request range end
                ctx->total_size = bytes_range->total_size;

                if (ctx->complete_range_end >= ctx->total_size)
                {
                        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                                      "illegal range, %O->%O/%O",
                                      ctx->complete_range_end, ctx->request_range_end,
                                      ctx->total_size);
                        return NGX_ERROR;
                }

                if (ctx->request_range_end == NGX_CONF_UNSET)
                {
                        ctx->request_range_end = ctx->total_size;
                }
                else
                        ctx->request_range_end = ngx_min(ctx->request_range_end, ctx->total_size);

                ctx->rest_range_length = bytes_range->last - bytes_range->head;
                ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0, "resp range span=%O",
                              ctx->rest_range_length);

                r->headers_out.content_length_n =
                        ctx->request_range_end - ctx->complete_range_end;
                
                r->headers_in.range = (ngx_table_elt_t *)ngx_list_push(&r->headers_in.headers);
                if (r->headers_in.range == NULL)
                        return NGX_ERROR;

                r->headers_in.range->hash = 1;
                ngx_str_set(&r->headers_in.range->key, "Content-Range");

                ngx_str_t *value_str = &r->headers_in.range->value;
                value_str->data = (u_char *)ngx_pnalloc(r->pool,
                                                        sizeof("bytes=") + 4 + 3 * NGX_OFF_T_LEN);

                value_str->len = ngx_sprintf(value_str->data,
                                             "bytes %O-%O/%O",
                                             ctx->complete_range_end, ctx->request_range_end - 1,       //note, -1
                                             ctx->total_size) - value_str->data;

                ctx->complete_range_end = bytes_range->last;//netx range
        }
        else {
                if (ctx->request_range_set)
                {
                        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "no ranges in resp, but req have!");
                        return NGX_HTTP_UPSTREAM_INVALID_HEADER;
                }

                ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0, "no ranges in resp");

                if (http_resp.http_body_len != NGX_CONF_UNSET)
                {
                        r->headers_out.content_length_n = http_resp.http_body_len;
                }
                else
                        ngx_http_clear_content_length(r);
        }

        KvMap& cookies = http_resp.cookies_set;
        if (!cookies.empty())
        {
                ngx_table_elt_t *set_cookie = (ngx_table_elt_t *)ngx_list_push(&r->headers_out.headers); //must set headers_out
                if (set_cookie == NULL)
                {
                        return NGX_ERROR;
                }

                set_cookie->hash = 1;
                ngx_str_set(&set_cookie->key, "Set-Cookie");
                set_cookie->value.len = 0;

                for (KvMap::iterator iter = cookies.begin(); iter != cookies.end(); ++iter)
                {
                        set_cookie->value.len += iter->first.length() + iter->second.length() + 2;
                }

                set_cookie->value.data = (u_char *)ngx_pnalloc(r->pool, set_cookie->value.len);
                if (set_cookie->value.data == NULL)
                {
                        return NGX_ERROR;
                }

                u_char *pos = set_cookie->value.data;
                for (KvMap::iterator iter = cookies.begin(); iter != cookies.end(); ++iter)
                {
                        pos = ngx_sprintf(pos, "%s=%s;", iter->first.c_str(), iter->second.c_str());
                }
                set_cookie->value.len = pos - set_cookie->value.data;
        }

        KvMap& other_headers = http_resp.other_heads;
        ngx_uint_t key_hash;
        u_char *low_key, *org_key;
        ngx_table_elt_t *head_elt;
        for (KvMap::iterator iter = other_headers.begin(); iter != other_headers.end(); ++iter)
        {
                if (iter->first.empty())
                        continue;

                org_key = (u_char *)iter->first.c_str();
                key_hash = ngx_hash_key_lc(org_key, iter->first.length());

                low_key = (u_char *)ngx_pnalloc(r->pool, iter->first.length());
                if (low_key == NULL)
                {
                        return NGX_ERROR;
                }
                ngx_strlow(low_key, org_key, iter->first.length());

                if (ngx_hash_find(&u->conf->hide_headers_hash,
                                  key_hash, low_key, iter->first.length()))
                        continue;

                head_elt = (ngx_table_elt_t *)ngx_list_push(&r->headers_out.headers);
                if (head_elt == NULL)
                {
                        return NGX_ERROR;
                }

                head_elt->hash = 1;
                head_elt->key.data = org_key;
                head_elt->key.len = iter->first.length();
                head_elt->value.data = (u_char *)iter->second.c_str();
                head_elt->value.len = iter->second.length();
        }

        if (ctx->last_datagram_chunk) //no content, just head !!?? !!
        {
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "just http head, shit !");
                r->headers_out.content_length_n = 0;
                r->header_only = 1;
        }
        else {
                ctx->head_buf.start = (u_char *)ngx_pnalloc(r->pool,
                                                            mlcf->upstream.buffer_size);

                ctx->head_buf.last = ctx->head_buf.pos = ctx->head_buf.start;
                ctx->head_buf.end = ctx->head_buf.start + mlcf->upstream.buffer_size;
                ctx->head_buf.memory = 1;

                ctx->decode_status = NGX_XDRIVE_PROXY_WAIT_HTTP_BODY_HEAD;
        }

        ctx->proccess_http_head_resp = ngx_http_proccess_post_head_resp;

        return NGX_OK;
}


static ngx_int_t
ngx_http_proccess_post_head_resp(ngx_http_request_t *r, HttpHeadResp *resp)
{
        ngx_http_upstream_t *u;
        ngx_http_xdrive_download_ctx_t *ctx;
        HttpHeadResp& http_resp = *resp;
        HttpBytesRange *bytes_range;

        u = r->upstream;
        ctx = (ngx_http_xdrive_download_ctx_t *)ngx_http_get_module_ctx(r,
                                                                        ngx_http_xdrive_download_module);

        bytes_range = &http_resp.bytes_range;

        if (!bytes_range->range_set)
        {
                return NGX_HTTP_UPSTREAM_INVALID_HEADER;
        }

        if (bytes_range->head < 0 || bytes_range->last < 0
            || bytes_range->head != ctx->complete_range_end
            || bytes_range->last <= ctx->complete_range_end
            || bytes_range->last > ctx->total_size    //sick
            )
        {
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                              "illegal resp range, %O->%O/%O, req start=%O",
                              bytes_range->head, bytes_range->last, bytes_range->total_size,
                              ctx->complete_range_end);

                return NGX_HTTP_UPSTREAM_INVALID_HEADER;
        }

        ctx->complete_range_end = bytes_range->last;

        ctx->rest_range_length = bytes_range->last - bytes_range->head;
        ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0, "resp range span=%O, complete_range_end=%O",
                      ctx->rest_range_length, ctx->complete_range_end);

        if (ctx->last_datagram_chunk)
        {
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "just http head, shit !");
                return NGX_HTTP_UPSTREAM_INVALID_HEADER;
        }

        ctx->decode_status = NGX_XDRIVE_PROXY_WAIT_HTTP_BODY_HEAD;
        return NGX_OK;
}


static ngx_int_t
ngx_http_xdrive_download_process_header(ngx_http_request_t *r)
{
        return ngx_http_xdrive_download_process_header_buf(r, &r->upstream->buffer);
}


static ngx_int_t
ngx_http_xdrive_download_filter_init(void *data)
{
        ngx_http_xdrive_download_ctx_t *ctx = (ngx_http_xdrive_download_ctx_t *)data;

        ngx_http_upstream_t *u;

        u = ctx->request->upstream;

        return NGX_OK;
}


static ngx_int_t
ngx_http_xdrive_download_filter(void *data, ssize_t bytes)
{
        ngx_http_xdrive_download_ctx_t *ctx = (ngx_http_xdrive_download_ctx_t *)data;

        u_char *last;
        ngx_buf_t *b, *head_b;
        ngx_chain_t *cl, **ll;
        ngx_http_upstream_t *u;
        size_t rest_len;

        ngx_http_xdrive_download_loc_conf_t *mlcf;

        mlcf = (ngx_http_xdrive_download_loc_conf_t *)
               ngx_http_get_module_loc_conf(ctx->request, ngx_http_xdrive_download_module);

        u = ctx->request->upstream;
        b = &u->buffer;

        if (ctx->decode_status == NGX_XDRIVE_PROXY_WAIT_HTTP_HEAD_HEAD
            || ctx->decode_status == NGX_XDRIVE_PROXY_WAIT_HTTP_HEAD_BODY)
        {
                head_b = &ctx->head_buf;
                rest_len = ngx_min(head_b->end - head_b->last, bytes);
                last = head_b->last;
                head_b->last = ngx_copy(head_b->last, b->last, rest_len);

                ngx_int_t ret = ngx_http_xdrive_download_process_header_buf(ctx->request, head_b);
                if (ret == NGX_AGAIN)
                        return NGX_OK;
                else if (ret != NGX_OK)
                        return ret;

                rest_len = head_b->pos - last; //note
                assert(rest_len > 0);

                b->last += rest_len;
                bytes -= rest_len;

                //reset
                head_b->pos = head_b->last = head_b->start;
        }

next_chunk:
        if (bytes == 0)
                return NGX_OK;

        if (ctx->decode_status == NGX_XDRIVE_PROXY_WAIT_HTTP_BODY_HEAD)
        {
                head_b = &ctx->head_buf;
                assert(ngx_buf_size(head_b) < NGX_XDRIVE_DATAGRAM_HEADER_EXT);
                
                rest_len = ngx_min(NGX_XDRIVE_DATAGRAM_HEADER_EXT - ngx_buf_size(head_b), bytes);
                head_b->last = ngx_copy(head_b->last, b->last, rest_len);

                b->last += rest_len;
                bytes -= rest_len;

                if (ngx_buf_size(head_b) < NGX_XDRIVE_DATAGRAM_HEADER_EXT)
                        return NGX_OK;

                //enough head for decode head
                ngx_xdrive_datagram_header_t header;
                if (ngx_decode_header(head_b->pos, NGX_XDRIVE_DATAGRAM_HEADER,
                                      &header, ctx->request->connection->log) != NGX_OK)
                {
                        return NGX_ERROR;
                }

                if (header._status != 0)
                {
                        ngx_log_error(NGX_LOG_WARN, ctx->request->connection->log, 0,
                                      "xdrive_download boy ret status not ok in response = %d", header._status);
                        return NGX_ERROR;
                }

                ctx->rest_datagram_length = header._length - NGX_XDRIVE_DATAGRAM_HEADER_EXT;
                head_b->pos += NGX_XDRIVE_DATAGRAM_HEADER + THRIFT_FILED_BEGIN_LEN;
                ctx->last_datagram_chunk = *head_b->pos;

                ngx_log_error(NGX_LOG_DEBUG, ctx->request->connection->log, 0, 
                        "datagram boy len=%d, last_chunk=%d", 
                        ctx->rest_datagram_length, 
                        ctx->last_datagram_chunk);

                //reset for next body chunk decode
                head_b->pos = head_b->last = head_b->start;

                ctx->decode_status = NGX_XDRIVE_PROXY_WAIT_HTTP_BODY_BODY;
        }

        if (bytes == 0)
                return NGX_OK;

        if (ctx->decode_status == NGX_XDRIVE_PROXY_WAIT_HTTP_BODY_BODY)
        {
                rest_len = ngx_min(ctx->rest_datagram_length, bytes);

                for (cl = u->out_bufs, ll = &u->out_bufs; cl; cl = cl->next)
                {
                        ll = &cl->next;
                }

                cl = ngx_chain_get_free_buf(ctx->request->pool, &u->free_bufs);
                if (cl == NULL)
                {
                        return NGX_ERROR;
                }

                *ll = cl;

                cl->buf->flush = 1;
                cl->buf->memory = 1;
                cl->buf->tag = u->output.tag;

                cl->buf->pos = b->last;
                b->last += rest_len;
                cl->buf->last = b->last;

                bytes -= rest_len;

                ctx->rest_range_length -= rest_len;
                ctx->rest_datagram_length -= rest_len;

                if (ctx->rest_datagram_length > 0) //need more data
                { 
                        if (u->length != NGX_MAX_SIZE_T_VALUE)
                        {
                                u->length -= rest_len;
                        }
                        return NGX_OK;
                }

                //drop last byte
                cl->buf->last -= THRIFT_FILED_STOP_LEN_LEN;
                ctx->rest_range_length += THRIFT_FILED_STOP_LEN_LEN;
                if (u->length != NGX_MAX_SIZE_T_VALUE)
                {
                        u->length -= (rest_len - THRIFT_FILED_STOP_LEN_LEN);
                }

                if (ctx->last_datagram_chunk)
                {
                        if (!ctx->request_range_set)   //no range
                        {
                                ngx_log_error(NGX_LOG_DEBUG, ctx->request->connection->log, 0, "no range");
                                cl->buf->last_buf = 1;
                                return NGX_OK;
                        }

                        ngx_log_error(NGX_LOG_DEBUG, ctx->request->connection->log, 0, 
                                        "reset range length=%d, complete range end=%O, request_end=%O", 
                                        ctx->rest_range_length, 
                                        ctx->complete_range_end, 
                                        ctx->request_range_end);

                        if (ctx->rest_range_length != 0)
                        {
                                ngx_log_error(NGX_LOG_WARN, ctx->request->connection->log, 0,
                                              "xdrive_download last datagram chunk, but rest_range_length=%O",
                                              ctx->rest_range_length);
                                return NGX_ERROR;
                        }

                        if (ctx->complete_range_end < ctx->request_range_end)
                        {
                                ngx_log_error(NGX_LOG_DEBUG, ctx->request->connection->log, 0,
                                              "next range begin, start=%O",
                                              ctx->complete_range_end);

                                return ngx_http_xdrive_next_range(ctx);
                        }
                        else {
                                ngx_log_error(NGX_LOG_DEBUG, ctx->request->connection->log, 0, 
                                                "all range done, over");
                                cl->buf->last_buf = 1;
                                return NGX_OK;
                        }
                }
                else {
                        ctx->decode_status = NGX_XDRIVE_PROXY_WAIT_HTTP_BODY_HEAD;
                        goto  next_chunk;
                }
        }

        return NGX_OK;
}


static void
ngx_http_xdrive_check_dl_rate_handler(ngx_event_t *ev)
{
        ngx_connection_t *c;
        ngx_http_request_t *r;
        ngx_http_xdrive_download_ctx_t *ctx;

        c = (ngx_connection_t *)ev->data;
        r = (ngx_http_request_t *)c->data;

        ctx = (ngx_http_xdrive_download_ctx_t *)ngx_http_get_module_ctx(r,
                                                                        ngx_http_xdrive_download_module);

        if (ngx_http_xdrive_next_range(ctx) != NGX_OK)
        {
                ngx_http_upstream_finalize_sub_request(r, r->upstream,
                                                   NGX_HTTP_INTERNAL_SERVER_ERROR);
        }
}

static void
ngx_http_xdrive_send_request_handler(ngx_event_t *ev)
{
        ngx_int_t rc;
        ngx_connection_t *c;
        ngx_http_request_t *r;
        ngx_http_xdrive_download_ctx_t *ctx;
        ngx_http_upstream_t *u;

        c = (ngx_connection_t *)ev->data;
        r = (ngx_http_request_t *)c->data;
        u = r->upstream;

        if (ev->timedout)
        {
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "send request timeout !");
                
                ngx_http_upstream_finalize_sub_request(r, r->upstream,
                                                   NGX_HTTP_REQUEST_TIME_OUT);
                return;
        }

        ctx = (ngx_http_xdrive_download_ctx_t *)ngx_http_get_module_ctx(r,
                                                                        ngx_http_xdrive_download_module);

        rc = ngx_output_chain(&u->output, NULL);

        if (rc == NGX_ERROR)
        {
                ngx_http_upstream_finalize_sub_request(r, r->upstream,
                                                   NGX_HTTP_INTERNAL_SERVER_ERROR);
                return;
        }

        if (rc == NGX_AGAIN)
        {
                ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0,
                              "need send again !");

                ngx_add_timer(c->write, u->conf->send_timeout);

                if (ngx_handle_write_event(c->write, u->conf->send_lowat) != NGX_OK)
                {
                        ngx_http_upstream_finalize_sub_request(r, r->upstream,
                                                           NGX_HTTP_INTERNAL_SERVER_ERROR);
                        return;
                }
        }
        else {
                ngx_add_timer(c->read, u->conf->read_timeout);
        }
}


static ngx_int_t
ngx_http_xdrive_next_range(ngx_http_xdrive_download_ctx_t *ctx)
{
        ngx_int_t rc;
        ngx_http_request_t *r = ctx->request;

        ngx_connection_t *c = r->upstream->peer.connection;
        
        c->data = r;

        ngx_http_upstream_t *u;
        u = r->upstream;

        ngx_http_xdrive_download_loc_conf_t *mlcf;
        mlcf = (ngx_http_xdrive_download_loc_conf_t *)
               ngx_http_get_module_loc_conf(ctx->request, ngx_http_xdrive_download_module);

        int64_t rest_sent = ctx->complete_range_end - ctx->request_range_begin
                            - r->connection->sent;

        if (rest_sent > 0 && rest_sent > mlcf->download_range_size / 2)//note !!
        {
                ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0,
                              "rest to be sent bytes=%O", rest_sent);

                c->write->handler = ngx_http_xdrive_check_dl_rate_handler;
                ngx_add_timer(c->write, mlcf->download_rate_check_interval);

                if (c->read->timer_set)
                {
                        ngx_del_timer(c->read);
                }
                return NGX_OK;
        }

        HttpBytesRange *bytes_range = &ctx->http_head_req->bytes_range;

        bytes_range->head = ctx->complete_range_end;
        bytes_range->last = ngx_min(ctx->request_range_end,
                                    ngx_to_range_end(bytes_range->head, mlcf->download_range_size));
        
        if (ctx->request_range_end - bytes_range->last <= mlcf->download_range_size / 2)
        {
                bytes_range->last = ctx->request_range_end;
        }

        ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0,
                      "range request[%O-%O]", bytes_range->head, bytes_range->last);

        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0,
                       "http upstream send ext request");

        c->log->action = "sending ext request to upstream";

        static uint32_t seq = ngx_time();
        ngx_chain_t *cl;
        cl = ngx_datagram_encode(r->pool, r->connection->log, mlcf->upstream.buffer_size,
                                 ctx->http_head_req, ++seq, 0xf01);
        if (cl == NULL)
                return  NGX_ERROR;

        rc = ngx_output_chain(&u->output, cl);

        if (rc == NGX_ERROR)
        {
                return NGX_ERROR;
        }

        if (rc == NGX_AGAIN)
        {
                ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0,
                              "first need send again !");

                c->write->handler = ngx_http_xdrive_send_request_handler;

                ngx_add_timer(c->write, u->conf->send_timeout);

                if (ngx_handle_write_event(c->write, u->conf->send_lowat) != NGX_OK)
                {
                        return NGX_ERROR;
                }
        }
        else {
                ngx_add_timer(c->read, u->conf->read_timeout);
                ctx->decode_status = NGX_XDRIVE_PROXY_WAIT_HTTP_HEAD_HEAD;
        }

        return NGX_OK;
}


static void
ngx_http_xdrive_download_abort_request(ngx_http_request_t *r)
{
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "abort http xdrive_download request");
        return;
}


static void
ngx_http_xdrive_download_finalize_request(ngx_http_request_t *r, ngx_int_t download)
{
        ngx_http_xdrive_download_ctx_t *ctx;

        ctx = (ngx_http_xdrive_download_ctx_t *)ngx_http_get_module_ctx(r, ngx_http_xdrive_download_module);
        if (ctx->http_head_req)
        {
                ctx->http_head_req->~HttpHeadReq();
                ctx->http_head_req = NULL;
        }

        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "finalize http xdrive_download request");
        return;
}


static void
ngx_http_upstream_finalize_sub_request(ngx_http_request_t *r
                , ngx_http_upstream_t *u, ngx_int_t rc)
{
        ngx_time_t *tp;

        ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0,
                      "finalize http upstream request: %i", rc);

        if (u->cleanup)
        {
                *u->cleanup = NULL;
                u->cleanup = NULL;
        }

        if (u->resolved && u->resolved->ctx)
        {
                ngx_resolve_name_done(u->resolved->ctx);
                u->resolved->ctx = NULL;
        }

        if (u->state && u->state->response_sec)
        {
                tp = ngx_timeofday();
                u->state->response_sec = tp->sec - u->state->response_sec;
                u->state->response_msec = tp->msec - u->state->response_msec;
        }

        u->finalize_request(r, rc);

        if (u->peer.free)
        {
                u->peer.free(&u->peer, u->peer.data, 0);
        }

        if (u->peer.connection)
        {
                ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0,
                              "close http upstream connection: %d",
                              u->peer.connection->fd);
                ngx_close_connection(u->peer.connection);
        }

        u->peer.connection = NULL;

        rc = ngx_http_send_special(r, NGX_HTTP_LAST);

        ngx_http_finalize_request(r, rc);
}


static void *
ngx_http_xdrive_download_create_loc_conf(ngx_conf_t *cf)
{
        ngx_http_xdrive_download_loc_conf_t *conf;

        conf = (ngx_http_xdrive_download_loc_conf_t *)ngx_pcalloc(cf->pool,
                                                                  sizeof(ngx_http_xdrive_download_loc_conf_t));
        if (conf == NULL)
        {
                return NULL;
        }
        conf->download_rate_check_interval = NGX_CONF_UNSET_MSEC;
        conf->download_range_size = NGX_CONF_UNSET_SIZE;

        conf->upstream.connect_timeout = NGX_CONF_UNSET_MSEC;
        conf->upstream.send_timeout = NGX_CONF_UNSET_MSEC;
        conf->upstream.read_timeout = NGX_CONF_UNSET_MSEC;

        conf->upstream.buffer_size = NGX_CONF_UNSET_SIZE;

        conf->upstream.hide_headers = (ngx_array_t *)NGX_CONF_UNSET_PTR;
        conf->upstream.pass_headers = (ngx_array_t *)NGX_CONF_UNSET_PTR;

        /* the hardcoded values */
        conf->upstream.cyclic_temp_file = 0;
        conf->upstream.buffering = 0;
        conf->upstream.ignore_client_abort = 0;
        conf->upstream.send_lowat = 0;
        conf->upstream.bufs.num = 0;
        conf->upstream.busy_buffers_size = 0;
        conf->upstream.max_temp_file_size = 0;
        conf->upstream.temp_file_write_size = 0;
        conf->upstream.pass_request_headers = 0;
        conf->upstream.pass_request_body = 0;
        
        conf->download_speed_rate = NGX_CONF_UNSET_SIZE;

        return conf;
}


static char *
ngx_http_xdrive_download_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
        ngx_hash_init_t hash;

        ngx_http_xdrive_download_loc_conf_t *prev = (ngx_http_xdrive_download_loc_conf_t *)parent;
        ngx_http_xdrive_download_loc_conf_t *conf = (ngx_http_xdrive_download_loc_conf_t *)child;

        ngx_conf_merge_msec_value(conf->upstream.connect_timeout,
                                  prev->upstream.connect_timeout, 60000);

        ngx_conf_merge_msec_value(conf->upstream.send_timeout,
                                  prev->upstream.send_timeout, 60000);

        ngx_conf_merge_msec_value(conf->upstream.read_timeout,
                                  prev->upstream.read_timeout, 60000);

        ngx_conf_merge_msec_value(conf->download_rate_check_interval,
                                  prev->download_rate_check_interval, 600); //0.6s

        ngx_conf_merge_size_value(conf->download_range_size,
                                  prev->download_range_size,
                                  (size_t)(128 * 1024)); //128k

        ngx_conf_merge_size_value(conf->download_speed_rate,
                                  prev->download_speed_rate,
                                  0);                                  

        ngx_conf_merge_size_value(conf->upstream.buffer_size,
                                  prev->upstream.buffer_size,
                                  (size_t)ngx_pagesize);

        ngx_conf_merge_bitmask_value(conf->upstream.next_upstream,
                                     prev->upstream.next_upstream,
                                     (NGX_CONF_BITMASK_SET
                                      | NGX_HTTP_UPSTREAM_FT_ERROR
                                      | NGX_HTTP_UPSTREAM_FT_TIMEOUT));

        if (conf->upstream.next_upstream & NGX_HTTP_UPSTREAM_FT_OFF)
        {
                conf->upstream.next_upstream = NGX_CONF_BITMASK_SET
                                               | NGX_HTTP_UPSTREAM_FT_OFF;
        }

        if (conf->upstream.upstream == NULL)
        {
                conf->upstream.upstream = prev->upstream.upstream;
        }

        hash.max_size = 512;
        hash.bucket_size = ngx_align(64, ngx_cacheline_size);
        hash.name = "xdrive_download_hide_headers_hash";

        if (NGX_OK != ngx_http_upstream_hide_headers_hash(cf, &conf->upstream,
                                                          &prev->upstream, ngx_http_xdrive_download_hide_headers, &hash))
        {
                return (char *)(NGX_CONF_ERROR);
        }

        return NGX_CONF_OK;
}


static char *
ngx_http_xdrive_download_pass(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
        ngx_http_xdrive_download_loc_conf_t *mlcf = (ngx_http_xdrive_download_loc_conf_t *)conf;

        ngx_str_t *value;
        ngx_url_t u;
        ngx_http_core_loc_conf_t *clcf;

        if (mlcf->upstream.upstream)
        {
                return "is duplicate";
        }

        value = (ngx_str_t *)cf->args->elts;

        ngx_memzero(&u, sizeof(ngx_url_t));

        u.url = value[1];
        u.no_resolve = 1;

        mlcf->upstream.upstream = ngx_http_upstream_add(cf, &u, 0);
        if (mlcf->upstream.upstream == NULL)
        {
                return (char *)(NGX_CONF_ERROR);
        }

        clcf = (ngx_http_core_loc_conf_t *)ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);

        clcf->handler = ngx_http_xdrive_download_handler;

        if (clcf->name.data[clcf->name.len - 1] == '/')
        {
                clcf->auto_redirect = 1;
        }

        return NGX_CONF_OK;
}

