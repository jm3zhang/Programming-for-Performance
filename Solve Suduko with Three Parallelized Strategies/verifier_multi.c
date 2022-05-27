#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <getopt.h>
#include "common.h"
#define MAX_WAIT_MSECS 30*1000 /* Wait max. 30 seconds */

/* Check the common header for the definition of puzzle */

#define URL "http://berkeley.uwaterloo.ca:4590/verify"
#define ROW_LENGTH 20
#define MATRIX_LENGTH 202

FILE *inputfile;

int total_puzzles = 0;
int num_connections = 1;
int http_status_code;

CURLM *cm = NULL;
const char *szUrl;

const char *ROW_FORMAT = "[%d,%d,%d,%d,%d,%d,%d,%d,%d]";
const char *MATRIX_FORMAT = "{\"content\":[%s, %s, %s, %s, %s, %s, %s, %s, %s]}";

/* Create cURL easy handle and configure it */
void *create_multi_eh(const int *result_code, const char *json_to_send, const struct curl_slist *headers);

/* Configure headers for the cURL request */
struct curl_slist *config_headers();

/* Transform the puzzle into an appropriate json format for the server */
char *convert_to_json(puzzle *p);

/* cURL read callback */
size_t read_callback(char *buffer, size_t size, size_t nitems, void *userdata);

/* cURL write callback */
size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata);

int verify_parallel();

size_t cb(char *d, size_t n, size_t l, void *p) {
    /* take care of the data here, ignored in this example */
    return n*l;
}

// void init( CURLM *cm, int i );

// int verify(puzzle *p) {
//     int *result = malloc(sizeof(int));
//     char *converted = convert_to_json(p);
//     struct curl_slist *headers = config_headers();
//     CURL *eh = create_eh(result, converted, headers);

//     CURLcode res = curl_easy_perform(eh);
//     if (res != CURLE_OK) {
//         printf("Error occurred in executing the cURL request: %s\n",
//                curl_easy_strerror(res));
//         exit(EXIT_FAILURE);
//     }
//     long response_code;
//     curl_easy_getinfo(eh, CURLINFO_RESPONSE_CODE, &response_code);
//     if (response_code != 200) {
//         printf("Error in HTTP request; HTTP code %lu received.\n", response_code);
//     }

//     curl_easy_cleanup(eh);
//     curl_slist_free_all(headers);
//     int ret = *result;
//     free(result);
//     free(converted);
//     return ret;
// }

int main(int argc, char **argv) {
    /* Parse arguments */
    int c;
    char* filename = NULL;
    while ((c = getopt(argc, argv, "t:i:")) != -1) {
        switch (c) {
            case 't':
                num_connections = strtoul(optarg, NULL, 10);
                if (num_connections == 0) {
                    printf("%s: option requires an argument > 0 -- 't'\n", argv[0]);
                    return EXIT_FAILURE;
                }
                break;
            case 'i':
                filename = optarg;
                break;
            default:
                return -1;
        }
    }

    /* Open file */
    inputfile = fopen(filename, "r");
    if (inputfile == NULL) {
        printf("Unable to open file!\n");
        return EXIT_FAILURE;
    }

    curl_global_init(CURL_GLOBAL_ALL);

    /* Check puzzles */
    int total_verify = verify_parallel();

    // printf("total_verify = %d\n", total_verify);

    printf("%d of %d puzzles passed verification.\n", total_verify, total_puzzles);
    curl_global_cleanup();
    fclose( inputfile );
    return 0;
}

int verify_parallel() {
    /* Check puzzles */
    CURL *eh = NULL;
    int verified = 0;
    int eh_idx = 0;
    puzzle *p;
    int still_running = 0;
    // int *result = malloc(sizeof(int));
    int result[num_connections];
    CURLcode return_code = 0;
    struct curl_slist *headers[num_connections];
    char *converted;

    curl_global_init( CURL_GLOBAL_ALL );
    cm = curl_multi_init();
    
    // // How to collect results ??? 

    // // How to free header ??? 

    // printf("1");

    while ((p = read_next_puzzle(inputfile)) != NULL) {
        total_puzzles++;
        converted = convert_to_json(p);
        headers[eh_idx] = config_headers();;

        //create easy handler
        //Add easy handler to multi handle
        create_multi_eh(&result[eh_idx], converted, headers[eh_idx]);

        if (eh_idx == num_connections - 1) {
            // wait num_connections to finish (wait till still_running = 0)
            do {
                int numfds = 0;
                int res = curl_multi_wait( cm, NULL, 0, MAX_WAIT_MSECS, &numfds );
                if( res != CURLM_OK ) {
                    fprintf( stderr, "error: curl_multi_wait() returned %d\n", res );
                    return EXIT_FAILURE;
                }
                curl_multi_perform( cm, &still_running );
            } while ( still_running );

            for (int i = 0; i < eh_idx + 1; i++){
                // printf("%d\n", result[i]);
                verified += result[i];
                result[i] = 0;
                // free(headers[i]);
                curl_slist_free_all(headers[i]);
            }

            eh_idx = 0;
        } else {
            eh_idx ++;
        }

        // verified += verify(p);
        free(p);
    } 

    if (eh_idx != 0){
        // wait num_connections to finish (wait till still_running = 0)
        do {
            int numfds = 0;
            int res = curl_multi_wait( cm, NULL, 0, MAX_WAIT_MSECS, &numfds );
            if( res != CURLM_OK ) {
                fprintf( stderr, "error: curl_multi_wait() returned %d\n", res );
                return EXIT_FAILURE;
            }
            curl_multi_perform( cm, &still_running );
        } while ( still_running );

        // collect your result 
        for (int i = 0; i < eh_idx; i++){
            verified += result[i];
            result[i] = 0;
            // free(headers[i]);
            curl_slist_free_all(headers[i]);
        } 
    }

    CURLMsg *msg = NULL;
    int msgs_left = 0;


    // collect result
    while ( ( msg = curl_multi_info_read( cm, &msgs_left ) ) ) {
        if ( msg->msg == CURLMSG_DONE ) {
            eh = msg->easy_handle;
            return_code = msg->data.result;
            // printf("%d", return_code);
            if ( return_code != CURLE_OK ) {
                fprintf( stderr, "CURL error code: %d\n", msg->data.result );
                continue;
            }

            // Get HTTP status code
            http_status_code = 0;
            szUrl = NULL;
            curl_easy_getinfo( eh, CURLINFO_RESPONSE_CODE, &http_status_code );
            curl_easy_getinfo( eh, CURLINFO_PRIVATE, &szUrl );
            if( http_status_code == 200 ) {
                // printf( "200 OK for %s\n", szUrl ) ;
            } else {
                fprintf( stderr, "GET of %s returned http status code %d\n", szUrl, http_status_code );
            }
            curl_multi_remove_handle( cm, eh );
            curl_easy_cleanup( eh );
        } else {
            fprintf( stderr, "error: after curl_multi_info_read(), CURLMsg=%d\n", msg->msg );
        }
    }
    
    curl_multi_cleanup(cm);
    return verified;
}

char *convert_to_json(puzzle *p) {
    char *rows[9];
    for (int i = 0; i < 9; i++) {
        rows[i] = malloc(ROW_LENGTH);
        memset(rows[i], 0, ROW_LENGTH);
        int written = sprintf(rows[i], ROW_FORMAT,
                              p->content[i][0], p->content[i][1], p->content[i][2],
                              p->content[i][3], p->content[i][4], p->content[i][5],
                              p->content[i][6], p->content[i][7], p->content[i][8]);
        if (written != ROW_LENGTH - 1) {
            printf("Something went wrong when writing row; expected to write %d but wrote %d...\n",
                    ROW_LENGTH -1, written);
        }
    }
    char *json = malloc(MATRIX_LENGTH);
    memset(json, 0, MATRIX_LENGTH);
    int written = sprintf(json, MATRIX_FORMAT,
                          rows[0], rows[1], rows[2], rows[3], rows[4],
                          rows[5], rows[6], rows[7], rows[8]);
    if (written != MATRIX_LENGTH - 1) {
        printf("Something went wrong when writing matrix; expected to write %d but wrote %d...\n",
               MATRIX_LENGTH -1, written);
    }
    for (int i = 0; i < 9; i++) {
        free(rows[i]);
    }
    return json;
}

size_t read_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
    memcpy(buffer, userdata, MATRIX_LENGTH);
    return MATRIX_LENGTH;
}

size_t write_callback(char *ptr, size_t size, size_t nmemb, void  *userdata) {
    printf("Write callback message from server: %s\n", ptr);
    int * p = (int*) userdata;
    *p = atoi(ptr);
    return size * nmemb;
}

struct curl_slist *config_headers() {
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Expect:");
    return headers;
}

void *create_multi_eh(const int *result, const char *json_to_send, const struct curl_slist *headers) {
    CURL *eh = curl_easy_init();
    curl_easy_setopt(eh, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(eh, CURLOPT_URL, URL);
    curl_easy_setopt(eh, CURLOPT_POST, 1L);
    curl_easy_setopt(eh, CURLOPT_READFUNCTION, read_callback);
    curl_easy_setopt(eh, CURLOPT_READDATA, json_to_send);
    curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(eh, CURLOPT_WRITEDATA, result);
    curl_easy_setopt(eh, CURLOPT_POSTFIELDSIZE, MATRIX_LENGTH);
    curl_multi_add_handle( cm, eh );
}

// void init( CURLM *cm, int i ) {
//     CURL *eh = curl_easy_init();
//     curl_easy_setopt( eh, CURLOPT_WRITEFUNCTION, cb );
//     curl_easy_setopt( eh, CURLOPT_HEADER, 0L );
//     curl_easy_setopt( eh, CURLOPT_URL, URL );
//     curl_easy_setopt( eh, CURLOPT_PRIVATE, URL) ;
//     curl_easy_setopt( eh, CURLOPT_VERBOSE, 0L );
//     curl_multi_add_handle( cm, eh );
// }

