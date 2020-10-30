#include "gw_body.h"
#include "c_icap/simple_api.h"
#include "../../common.h"
#include <assert.h>

void gw_body_data_new(gw_body_data_t *bd, int size)
{
    bd->store = ci_simple_file_new(size);   
    bd->rebuild = ci_simple_file_new(0);
    bd->buf_exceed = 0;
    bd->decoded = NULL;
}

void gw_body_data_named(gw_body_data_t *bd, const char *dir, const char *name)
{
    bd->store = ci_simple_file_named_new((char *)dir, (char *)name, 0);
    bd->buf_exceed = 0;
}

void gw_body_data_destroy(gw_body_data_t *body)
{
    ci_simple_file_destroy(body->store);
    body->store = NULL;

    if (body->decoded) {
        ci_simple_file_destroy(body->decoded);
        body->decoded = NULL;
    }
    if (body->rebuild) {
        ci_simple_file_destroy(body->rebuild);
        body->rebuild = NULL;        
    }    
}

void gw_body_data_release(gw_body_data_t *body)
{
    ci_simple_file_release(body->store);
    body->store = NULL;
    ci_simple_file_release(body->rebuild);
    
    if (body->decoded) {
        ci_simple_file_destroy(body->decoded);
        body->decoded = NULL;
    }
}

int gw_body_data_write(gw_body_data_t *body, char *buf, int len, int iseof)
{
    return ci_simple_file_write(body->store, buf, len, iseof);
}

int gw_body_data_read(gw_body_data_t *body, char *buf, int len)
{
    return ci_simple_file_read(body->store, buf, len);
}

void gw_body_data_replace_body(gw_body_data_t *body, char *buf, int len)
{
	gw_body_data_destroy(body);
	gw_body_data_new(body, len);
	gw_body_data_write(body, buf, len, 1);	
}

int gw_decompress_to_simple_file(int encodeMethod, const char *inbuf, size_t inlen, struct ci_simple_file *outfile, ci_off_t max_size)
{
#if defined(HAVE_CICAP_DECOMPRESS_TO)
    return ci_decompress_to_simple_file(encodeMethod, inbuf, inlen, outfile, max_size);
#else
    if (encodeMethod == CI_ENCODE_GZIP || encodeMethod == CI_ENCODE_DEFLATE)
        return ci_inflate_to_simple_file(inbuf, inlen, outfile, max_size);
    else if (encodeMethod == CI_ENCODE_BZIP2)
        return ci_bzunzip_to_simple_file(inbuf, inlen, outfile, max_size);
#if defined(HAVE_CICAP_BROTLI)
    else if (encodeMethod == CI_ENCODE_BROTLI)
        return ci_brinflate_to_simple_file(inbuf, inlen, outfile, max_size);
#endif
#endif
    return CI_UNCOMP_ERR_ERROR;
}
