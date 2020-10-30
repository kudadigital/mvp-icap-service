#ifndef gw_body_data_H
#define gw_body_data_H

#include "body.h"

enum gw_body_type {GW_BT_NONE=0, GW_BT_FILE, GW_BT_MEM};

typedef struct gw_body_data {
    ci_simple_file_t *store;
    ci_simple_file_t* rebuild;
    int buf_exceed;
    ci_simple_file_t *decoded;
} gw_body_data_t;

#define gw_body_data_lock_all(bd) (void)(ci_simple_file_lock_all((bd)->store))
#define gw_body_data_unlock(bd, len) (void)(ci_simple_file_unlock((bd)->store, len))
#define gw_body_data_unlock_all(bd) (void)(ci_simple_file_unlock_all((bd)->store))
#define gw_body_data_size(bd) ((bd)->store->endpos)
#define gw_body_rebuild_size(bd) ((bd)->rebuild->endpos)

void gw_body_data_new(gw_body_data_t *bd, int size);
void gw_body_data_named(gw_body_data_t *bd, const char *dir, const char *name);
void gw_body_data_destroy(gw_body_data_t *body);
void gw_body_data_release(gw_body_data_t *body);
int gw_body_data_write(gw_body_data_t *body, char *buf, int len, int iseof);
int gw_body_data_read(gw_body_data_t *body, char *buf, int len);
void gw_body_data_replace_body(gw_body_data_t *body, char *buf, int len);

int gw_decompress_to_simple_file(int encodingMethod, const char *inbuf, size_t inlen, struct ci_simple_file *outfile, ci_off_t max_size);
#endif
