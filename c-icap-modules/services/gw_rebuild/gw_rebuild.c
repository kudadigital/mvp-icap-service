#include <wchar.h>
#include "c_icap/c-icap.h"
#include "c_icap/service.h"
#include "c_icap/header.h"
#include "c_icap/simple_api.h"
#include "c_icap/debug.h"
#include "c_icap/cfg_param.h"
#include "c_icap/filetype.h"
#include "c_icap/ci_threads.h"
#include "c_icap/mem.h"
#include "c_icap/commands.h"
#include "c_icap/txt_format.h"
#include "c_icap/txtTemplate.h"
#include "c_icap/stats.h"
#include "gw_rebuild.h"
#include "gw_proxy_api.h"

#include "md5.h"
#include "common.h"
#include <assert.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>

void generate_error_page(gw_rebuild_req_data_t *data, ci_request_t *req);
static void rebuild_content_length(ci_request_t *req, gw_body_data_t *body);
static int file_exists (char *filename);
/***********************************************************************************/
/* Module definitions                                                              */

static int ALLOW204 = 1;
static ci_off_t MAX_OBJECT_SIZE = 5*1024*1024;
static int DATA_CLEANUP = 1;
#define GW_VERSION_SIZE 15
#define GW_BT_FILE_PATH_SIZE 150
#define STATS_BUFFER 1024

char *PROXY_APP_LOCATION = NULL;

char *REBUILD_VERSION = "2.1.1";

/*Statistic  Ids*/
static int GW_SCAN_REQS = -1;
static int GW_SCAN_BYTES = -1;
static int GW_REBUILD_FAILURES = -1;
static int GW_REBUILD_ERRORS = -1;
static int GW_REBUILD_SUCCESSES = -1;
static int GW_NOT_PROCESSED = -1;
static int GW_UNPROCESSABLE = -1;

/*********************/
/* Formating table   */
static int fmt_gw_rebuild_http_url(ci_request_t *req, char *buf, int len, const char *param);
static int fmt_gw_rebuild_error_code(ci_request_t *req, char *buf, int len, const char *param);

struct ci_fmt_entry gw_rebuild_report_format_table [] = {
    {"%GU", "The HTTP url", fmt_gw_rebuild_http_url},
    {"%GE", "The Error code", fmt_gw_rebuild_error_code},
    { NULL, NULL, NULL}
};

static ci_service_xdata_t *gw_rebuild_xdata = NULL;

static int GWREQDATA_POOL = -1;

static int gw_rebuild_init_service(ci_service_xdata_t *srv_xdata,
                           struct ci_server_conf *server_conf);
static int gw_rebuild_post_init_service(ci_service_xdata_t *srv_xdata,
                           struct ci_server_conf *server_conf);
static void gw_rebuild_close_service();
static int gw_rebuild_check_preview_handler(char *preview_data, int preview_data_len,
                                    ci_request_t *);
static int gw_rebuild_end_of_data_handler(ci_request_t *);
static void *gw_rebuild_init_request_data(ci_request_t *req);
static void gw_rebuild_release_request_data(void *srv_data);
static int gw_rebuild_io(char *wbuf, int *wlen, char *rbuf, int *rlen, int iseof,
                 ci_request_t *req);

/*Arguments parse*/
static void gw_rebuild_parse_args(gw_rebuild_req_data_t *data, char *args);

/*General functions*/
static void set_istag(ci_service_xdata_t *srv_xdata);
static void cmd_reload_istag(const char *name, int type, void *data);
static int init_body_data(ci_request_t *req);

/*Configuration Table .....*/
static struct ci_conf_entry conf_variables[] = {
    {"MaxObjectSize", &MAX_OBJECT_SIZE, ci_cfg_size_off, NULL},
    {"Allow204Responses", &ALLOW204, ci_cfg_onoff, NULL},
    {"DataCleanup", &DATA_CLEANUP, ci_cfg_onoff, NULL},
    {"ProxyAppLocation", &PROXY_APP_LOCATION, ci_cfg_set_str, NULL},
};

CI_DECLARE_MOD_DATA ci_service_module_t service = {
    "gw_rebuild",              /*Module name */
    "Glasswall Rebuild service",        /*Module short description */
    ICAP_RESPMOD | ICAP_REQMOD,        /*Service type response or request modification */
    gw_rebuild_init_service,    /*init_service. */
    gw_rebuild_post_init_service,   /*post_init_service. */
    gw_rebuild_close_service,   /*close_service */
    gw_rebuild_init_request_data,       /*init_request_data. */
    gw_rebuild_release_request_data,    /*release request data */
    gw_rebuild_check_preview_handler,
    gw_rebuild_end_of_data_handler,
    gw_rebuild_io,
    conf_variables,
    NULL
};

static char* concat(char* output, const char* s1, const char* s2);
static int gw_rebuild_init_service(ci_service_xdata_t *srv_xdata,
                           struct ci_server_conf *server_conf)
{   
    gw_rebuild_xdata = srv_xdata;

    ci_service_set_preview(srv_xdata, 1024);
    ci_service_enable_204(srv_xdata);
    ci_service_set_transfer_preview(srv_xdata, "*");

    /*Initialize object pools*/
    GWREQDATA_POOL = ci_object_pool_register("gw_rebuild_req_data_t", sizeof(gw_rebuild_req_data_t));

    if(GWREQDATA_POOL < 0) {
        ci_debug_printf(1, " gw_rebuild_init_service: error registering object_pool gw_rebuild_req_data_t\n");
        return CI_ERROR;
    }

    /*initialize statistic counters*/
    /* TODO:convert to const after fix ci_stat_* api*/
    char template_buf[STATS_BUFFER];
    char buf[STATS_BUFFER];
    buf[STATS_BUFFER-1] = '\0';
    char *stats_label = "Service gw_rebuild";
    concat(template_buf, stats_label, " %s");
    snprintf(buf, STATS_BUFFER-1, template_buf, "REQUESTS SCANNED");
    GW_SCAN_REQS = ci_stat_entry_register(buf, STAT_INT64_T, stats_label);
    snprintf(buf, STATS_BUFFER-1, template_buf, "BODY BYTES SCANNED");
    GW_SCAN_BYTES = ci_stat_entry_register(buf, STAT_KBS_T, stats_label);
    snprintf(buf, STATS_BUFFER-1, template_buf, "REBUILD FAILURES");    
    GW_REBUILD_FAILURES = ci_stat_entry_register(buf, STAT_INT64_T, stats_label);
    snprintf(buf, STATS_BUFFER-1, template_buf, "REBUILD ERRORS");
    GW_REBUILD_ERRORS = ci_stat_entry_register(buf, STAT_INT64_T, stats_label);
    snprintf(buf, STATS_BUFFER-1, template_buf, "SCAN REBUILT");
    GW_REBUILD_SUCCESSES = ci_stat_entry_register(buf, STAT_INT64_T, stats_label);
    snprintf(buf, STATS_BUFFER-1, template_buf, "UNPROCESSED");
    GW_NOT_PROCESSED = ci_stat_entry_register(buf, STAT_INT64_T, stats_label);
    snprintf(buf, STATS_BUFFER-1, template_buf, "UNPROCESSABLE");
    GW_UNPROCESSABLE = ci_stat_entry_register(buf, STAT_INT64_T, stats_label);      
        
    return CI_OK;
}


static int gw_rebuild_post_init_service(ci_service_xdata_t *srv_xdata,
                           struct ci_server_conf *server_conf)
{   
    if (!PROXY_APP_LOCATION){
       ci_debug_printf(1, "Proxy App location not specified\n");
       return CI_ERROR;
    }
    
    if (!file_exists(PROXY_APP_LOCATION)){
       ci_debug_printf(1, "Proxy App not found at %s\n", PROXY_APP_LOCATION);
       return CI_ERROR;   
    }    
    
    set_istag(gw_rebuild_xdata);
    register_command_extend(GW_RELOAD_ISTAG, ONDEMAND_CMD, NULL, cmd_reload_istag);

    ci_debug_printf(1, "Using Proxy App at %s\n", PROXY_APP_LOCATION);    
    return CI_OK;
}

static void gw_rebuild_close_service()
{
    ci_debug_printf(3, "gw_rebuild_close_service......\n");
    ci_object_pool_unregister(GWREQDATA_POOL);
}

static void *gw_rebuild_init_request_data(ci_request_t *req)
{
    int preview_size;
    gw_rebuild_req_data_t *data;

    ci_debug_printf(3, "gw_rebuild_init_request_data......\n");

     preview_size = ci_req_preview_size(req);

    if (req->args[0] != '\0') {
        ci_debug_printf(5, "service arguments:%s\n", req->args);
    }
    if (ci_req_hasbody(req)) {
        ci_debug_printf(5, "Request type: %d. Preview size:%d\n", req->type, preview_size);
        if (!(data = ci_object_pool_alloc(GWREQDATA_POOL))) {
            ci_debug_printf(1, "Error allocation memory for service data!!!!!!!\n");
            return NULL;
        }
        memset(&data->body,0, sizeof(gw_body_data_t));
        data->error_page = NULL;
        data->url_log[0] = '\0';
        data->gw_status = GW_STATUS_UNDEFINED;
        data->gw_processing = GW_PROCESSING_UNDEFINED;
        if (ALLOW204)
            data->args.enable204 = 1;
        else
            data->args.enable204 = 0;
        data->args.sizelimit = 1;
        data->args.mode = 0;

        if (req->args[0] != '\0') {
            ci_debug_printf(5, "service arguments:%s\n", req->args);
            gw_rebuild_parse_args(data, req->args);
        }
        if (data->args.enable204 && ci_allow204(req))
            data->allow204 = 1;
        else
            data->allow204 = 0;
        data->req = req;

        return data;
    }
    return NULL;
}

static void gw_rebuild_release_request_data(void *data)
{
    if (data) {
        ci_debug_printf(3, "Releasing gw_rebuild data.....\n");
        gw_rebuild_req_data_t *requestData = (gw_rebuild_req_data_t *) data;
        if (DATA_CLEANUP)
        {            
            gw_body_data_destroy(&requestData->body);
        }
        else
        {
            ci_debug_printf(3, "Leaving gw_rebuild data body.....\n");
        }

        if (((gw_rebuild_req_data_t *) data)->error_page)
            ci_membuf_free(((gw_rebuild_req_data_t *) data)->error_page);

        ci_object_pool_free(data);
     }
}

static int gw_rebuild_check_preview_handler(char *preview_data, int preview_data_len,
                                    ci_request_t *req)
{
     ci_off_t content_size = 0;

     gw_rebuild_req_data_t *data = ci_service_data(req);

     ci_debug_printf(3, "gw_rebuild_check_preview_handler; preview data size is %d\n", preview_data_len);

     if (!data || !ci_req_hasbody(req)){
        ci_debug_printf(6, "No body data, allow 204\n");
        ci_stat_uint64_inc(GW_UNPROCESSABLE, 1); 
        return CI_MOD_ALLOW204;
     }

    data->max_object_size = MAX_OBJECT_SIZE;

    /*Compute the expected size, will be used by must_scanned*/
    content_size = ci_http_content_length(req);
    data->expected_size = content_size;
    ci_debug_printf(6, "gw_rebuild_check_preview_handler: expected_size is %ld\n", content_size);

    /*log objects url*/
    if (!ci_http_request_url(req, data->url_log, LOG_URL_SIZE)) {
        ci_debug_printf(2, "Failed to retrieve HTTP request URL\n");
    }

    if (init_body_data(req) == CI_ERROR){
        ci_stat_uint64_inc(GW_REBUILD_ERRORS, 1);         
        return CI_ERROR;
    }
    
    if (preview_data_len == 0) {
        return CI_MOD_CONTINUE;
    }
    
    if (preview_data_len && 
        gw_body_data_write(&data->body, preview_data, preview_data_len, ci_req_hasalldata(req)) == CI_ERROR){
            ci_stat_uint64_inc(GW_REBUILD_ERRORS, 1);                        
            return CI_ERROR;
    }
    ci_debug_printf(6, "gw_rebuild_check_preview_handler: gw_body_data_write data_len %d\n", preview_data_len);

    return CI_MOD_CONTINUE;
}

int gw_rebuild_write_to_net(char *buf, int len, ci_request_t *req)
{
    ci_debug_printf(9, "gw_rebuild_write_to_net; buf len is %d\n", len);

    int bytes;
    gw_rebuild_req_data_t *data = ci_service_data(req);
    if (!data)
        return CI_ERROR;

    bytes = gw_body_data_read(&data->body, buf, len);

    ci_debug_printf(9, "gw_rebuild_write_to_net; write bytes is %d\n", bytes);

    return bytes;
}

int gw_rebuild_read_from_net(char *buf, int len, int iseof, ci_request_t *req)
{
    ci_debug_printf(9, "gw_rebuild_read_from_net; buf len is %d, iseof is %d\n", len, iseof);

    gw_rebuild_req_data_t *data = ci_service_data(req);
    if (!data)
        return CI_ERROR;

    if (data->args.sizelimit
        && gw_body_data_size(&data->body) >= data->max_object_size) {
        ci_debug_printf(2, "Object bigger than max scanable file. \n");

    /*TODO: Raise an error report rather than just raise an error */
    return CI_ERROR;
    } 
    ci_debug_printf(9, "gw_rebuild_read_from_net:Writing to data->body, %d bytes \n", len);

    return gw_body_data_write(&data->body, buf, len, iseof);
}

static int gw_rebuild_io(char *wbuf, int *wlen, char *rbuf, int *rlen, int iseof, ci_request_t *req)
{
    char printBuffer[100];
    char tempBuffer[20];
    printBuffer[0] = '\0';
    strcat(printBuffer, "gw_rebuild_io, ");

    if (wlen) {
        sprintf(tempBuffer, "wlen=%d, ", *wlen);
        strcat(printBuffer, tempBuffer);
    }
    if (rlen) {
        sprintf(tempBuffer, "rlen=%d, ", *rlen);
        strcat(printBuffer, tempBuffer);
    }
    sprintf(tempBuffer, "iseof=%d\n", iseof);
    strcat(printBuffer, tempBuffer);
    ci_debug_printf(9, "%s", printBuffer);

     if (rbuf && rlen) {
        *rlen = gw_rebuild_read_from_net(rbuf, *rlen, iseof, req);
        if (*rlen == CI_ERROR)
            return CI_ERROR;
            /*else if (*rlen < 0) ignore*/
     }
     else if (iseof && gw_rebuild_read_from_net(NULL, 0, iseof, req) == CI_ERROR){
         return CI_ERROR;
     }

     if (wbuf && wlen) {
          *wlen = gw_rebuild_write_to_net(wbuf, *wlen, req);
     }
     return CI_OK;
}

static int rebuild_request_body(ci_request_t *req, gw_rebuild_req_data_t* data, ci_simple_file_t* input, ci_simple_file_t* output);
static int gw_rebuild_end_of_data_handler(ci_request_t *req)
{
    ci_debug_printf(3, "gw_rebuild_end_of_data_handler\n");

    gw_rebuild_req_data_t *data = ci_service_data(req);

    if (!data){
        data->gw_processing = GW_PROCESSING_NONE;
        ci_stat_uint64_inc(GW_UNPROCESSABLE, 1);                 
        return CI_MOD_DONE;
    }

    int rebuild_status = CI_ERROR;
    rebuild_status = rebuild_request_body(req, data, data->body.store, data->body.rebuild);
    
    if (rebuild_status == CI_ERROR){
        int error_report_size;
        generate_error_page(data, req);               
        error_report_size = ci_membuf_size(data->error_page);
   
        gw_body_data_destroy(&data->body);
        gw_body_data_new(&data->body, error_report_size);
        gw_body_data_write(&data->body, data->error_page->buf, error_report_size, 1);
        rebuild_content_length(req, &data->body);
    }

    ci_debug_printf(3, "gw_rebuild_end_of_data_handler allow204(%d)\n", data->allow204);
    if (data->allow204 && rebuild_status == CI_MOD_ALLOW204){
        ci_debug_printf(3, "gw_rebuild_end_of_data_handler returning %d\n", rebuild_status);
        return CI_MOD_ALLOW204;
    }
 
    ci_req_unlock_data(req);
    gw_body_data_unlock_all(&data->body);

    return CI_MOD_DONE;
}

static int call_proxy_application(ci_simple_file_t* input, ci_simple_file_t* output);
static int replace_request_body(gw_rebuild_req_data_t* data, ci_simple_file_t* rebuild);
static int refresh_externally_updated_file(ci_simple_file_t* updated_file);
/* Return value:  */
/* CI_OK - to continue to rebuilt content */
/* CI_MOD_ALLOW204 - to continue to unchanged content */
/* CI_ERROR - to report error, whether due to policy error or processing error */
int rebuild_request_body(ci_request_t *req, gw_rebuild_req_data_t* data, ci_simple_file_t* input, ci_simple_file_t* output)
{
    ci_stat_uint64_inc(GW_SCAN_REQS, 1);    
    ci_stat_kbs_inc(GW_SCAN_BYTES, (int)gw_body_data_size(&data->body));
    int gw_proxy_api_return = call_proxy_application(input, output);
    
    /* Store the return status for inclusion in any error report */
    data->gw_status = gw_proxy_api_return;
    
    int ci_status;
    switch (gw_proxy_api_return)
    {
        case GW_FAILED:
            ci_debug_printf(3, "rebuild_request_body GW_FAILED\n");
            ci_stat_uint64_inc(GW_REBUILD_FAILURES, 1); 
            ci_status = CI_ERROR;
            break;
        case GW_ERROR:
            ci_debug_printf(3, "rebuild_request_body GW_ERROR\n");
            ci_stat_uint64_inc(GW_REBUILD_ERRORS, 1); 
            ci_status = CI_ERROR;
            break;
        case GW_UNPROCESSED:
            ci_debug_printf(3, "rebuild_request_body GW_UNPROCESSED\n");
            ci_stat_uint64_inc(GW_NOT_PROCESSED, 1); 
            ci_status = CI_MOD_ALLOW204;
            break;
        case GW_REBUILT:
            {
                ci_debug_printf(3, "rebuild_request_body GW_REBUILT\n");
                
                if (refresh_externally_updated_file(output) == CI_ERROR){
                    ci_debug_printf(3, "Problem sizing Rebuild\n");
                    ci_stat_uint64_inc(GW_REBUILD_ERRORS, 1); 
                    ci_status = CI_ERROR;
                    break;
                } 
                if (ci_simple_file_size(output) == 0){
                    ci_debug_printf(3, "No Rebuilt document available\n");
                    ci_stat_uint64_inc(GW_REBUILD_ERRORS, 1); 
                    ci_status =  CI_ERROR;
                    break;
                }
                if (!replace_request_body(data, output)){
                    ci_debug_printf(3, "Error replacing request body\n");
                    ci_stat_uint64_inc(GW_REBUILD_ERRORS, 1); 
                    ci_status =  CI_ERROR;
                    break;
                }      
                rebuild_content_length(req, &data->body);  
                ci_stat_uint64_inc(GW_REBUILD_SUCCESSES, 1);           
                ci_status =  CI_OK;
                break;
            }
        
        default:
            ci_debug_printf(3, "Unrecognised Proxy API return value (%d)\n", gw_proxy_api_return);
            ci_stat_uint64_inc(GW_REBUILD_ERRORS, 1); 
            ci_status =  CI_ERROR;        
    }
    return ci_status;    
}

int replace_request_body(gw_rebuild_req_data_t* data, ci_simple_file_t* rebuild)
{
    ci_simple_file_destroy(data->body.store);
    data->body.store = rebuild;        
    return CI_OK;
}

/*******************************************************************************/
/* Other  functions                                                            */

static void cmd_reload_istag(const char *name, int type, void *data)
{
    if (gw_rebuild_xdata)
        set_istag(gw_rebuild_xdata);
}

void set_istag(ci_service_xdata_t *srv_xdata)
{
    ci_debug_printf(9, "Updating istag %s with %s\n", srv_xdata->ISTag, REBUILD_VERSION);
    char istag[SERVICE_ISTAG_SIZE + 1];
    
    istag[0] = '-';
    strncpy(istag + 1, REBUILD_VERSION, strlen(REBUILD_VERSION));
    ci_service_set_istag(srv_xdata, istag);
}

static int init_body_data(ci_request_t *req)
{
    gw_rebuild_req_data_t *data = ci_service_data(req);
    assert(data);

    gw_body_data_new(&(data->body), data->args.sizelimit==0 ? 0 : data->max_object_size);
        /*Icap server can not send data at the begining.
        The following call does not needed because the c-icap
        does not send any data if the ci_req_unlock_data is not called:*/
        /* ci_req_lock_data(req);*/

        /* Let ci_simple_file api to control the percentage of data.
         For now no data can send */
    gw_body_data_lock_all(&(data->body));

    return CI_OK;
}

void generate_error_page(gw_rebuild_req_data_t *data, ci_request_t *req)
{
    ci_membuf_t *error_page;
    char buf[1024];
    const char *lang;

    if ( ci_http_response_headers(req))
         ci_http_response_reset_headers(req);
    else
         ci_http_response_create(req, 1, 1);
    ci_http_response_add_header(req, "HTTP/1.0 403 Forbidden");
    ci_http_response_add_header(req, "Server: C-ICAP");
    ci_http_response_add_header(req, "Connection: close");
    ci_http_response_add_header(req, "Content-Type: text/html");

    error_page = ci_txt_template_build_content(req, "gw_rebuild", "POLICY_ISSUE",
                           gw_rebuild_report_format_table);

    lang = ci_membuf_attr_get(error_page, "lang");
    if (lang) {
        snprintf(buf, sizeof(buf), "content-language: %s", lang);
        buf[sizeof(buf)-1] = '\0';
        ci_http_response_add_header(req, buf);
    }
    else
        ci_http_response_add_header(req, "Content-Language: en");

    data->error_page = error_page;
}

/***************************************************************************************/
/* Parse arguments function -
   Current arguments: allow204=on|off, sizelimit=off
*/
void gw_rebuild_parse_args(gw_rebuild_req_data_t *data, char *args)
{
     char *str;
     if ((str = strstr(args, "allow204="))) {
          if (strncmp(str + 9, "on", 2) == 0)
               data->args.enable204 = 1;
          else if (strncmp(str + 9, "off", 3) == 0)
               data->args.enable204 = 0;
     }

     if ((str = strstr(args, "sizelimit="))) {
          if (strncmp(str + 10, "off", 3) == 0)
               data->args.sizelimit = 0;
     }

}

static int exec_prog(const char **argv);
/* Return value: exit status from executed application (gw_proxy_api_return), or GW_ERROR */
int call_proxy_application(ci_simple_file_t* input, ci_simple_file_t* output)
{     
    const char* args[6] = {PROXY_APP_LOCATION, 
                           "-i", input->filename, 
                           "-o", output->filename, 
                           NULL};
    return exec_prog(args);  
}

/* First array item is path to executable, last array item is null. Program arguments are intermediate array elements*/
/* Return value: exit status from executed application (gw_proxy_api_return), or GW_ERROR */
static int exec_prog(const char **argv)
{
    pid_t   my_pid;
    int     status, timeout;

    if (0 == (my_pid = fork())) {
        if (-1 == execvp(argv[0], (char **)argv)) {
            ci_debug_printf(1, "child process execve failed for %s (%d)", argv[0], my_pid);
            return GW_ERROR;
        }
    }
    timeout = 1000;

    while (0 == waitpid(my_pid , &status , WNOHANG)) {
        if ( --timeout < 0 ) {
            ci_debug_printf(1, "Unexpected timeout running Proxy application (%d)\n", my_pid);
            return GW_ERROR;
        }
        sleep(1);
    }

    ci_debug_printf(8, "%s PID %d WEXITSTATUS %d WIFEXITED %d [status %d]\n",
            argv[0], my_pid, WEXITSTATUS(status), WIFEXITED(status), status);
            
    if (WIFEXITED(status) ==0)
    {
        ci_debug_printf(1, "Unexpected error running Proxy application (%d)\n", status);
        return GW_ERROR;
    }

    return WEXITSTATUS(status);
}

void rebuild_content_length(ci_request_t *req, gw_body_data_t *bd)
{
    ci_off_t new_file_size = 0;
    char buf[256];
    ci_simple_file_t *body = NULL;

    body = bd->store;
    assert(body->readpos == 0);
    new_file_size = body->endpos;

    ci_debug_printf(5, "Body data size changed to new size %"  PRINTF_OFF_T "\n",
                    (CAST_OFF_T)new_file_size);

    snprintf(buf, sizeof(buf), "Content-Length: %" PRINTF_OFF_T, (CAST_OFF_T)new_file_size);
    int remove_status = 0;
    if (req->type == ICAP_REQMOD){
        remove_status = ci_http_request_remove_header(req, "Content-Length");
        ci_http_request_add_header(req, buf);
        ci_debug_printf(5, "Request Header updated(%d), %s\n", remove_status, buf);        
    }
    else if (req->type == ICAP_RESPMOD){
        remove_status = ci_http_response_remove_header(req, "Content-Length");
        ci_http_response_add_header(req, buf);
        ci_debug_printf(5, "Response Header updated(%d), %s\n", remove_status, buf);        
    }   
}

static int file_size(int fd)
{
   struct stat s;
   if (fstat(fd, &s) == -1) {
      return(-1);
   }
   return(s.st_size);
}

int refresh_externally_updated_file(ci_simple_file_t* updated_file)
{
    ci_off_t new_size;
    ci_simple_file_write(updated_file, NULL, 0, 1);  /* to close of the file have been modified externally */
       
    new_size = file_size(updated_file->fd);
    if (new_size < 0)
        return CI_ERROR;
    
    updated_file->endpos= new_size;
    updated_file->readpos=0;
    return CI_OK;
}

/**************************************************************/
/* gw_rebuild templates  formating table                         */

int fmt_gw_rebuild_http_url(ci_request_t *req, char *buf, int len, const char *param)
{
    gw_rebuild_req_data_t *data = ci_service_data(req);
    return snprintf(buf, len, "%s", data->url_log);
}

static int fmt_gw_rebuild_error_code(ci_request_t *req, char *buf, int len, const char *param)
{
    gw_rebuild_req_data_t *data = ci_service_data(req);
    return snprintf(buf, len, "%d", data->gw_status);
}

char* concat(char* output, const char* s1, const char* s2)
{
    strcpy(output, s1);
    strcat(output, s2);
    return output;
}

static int file_exists (char *filename) {
  struct stat   buffer;   
  return (stat (filename, &buffer) == 0);
}