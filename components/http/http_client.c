#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "http_client.h"

#define TAG "HTTPCLIENT:"

#define HTTP_READ_TIMEOUT_S    (2)
#define HTTP_WRITE_TIMEOUT_S    (6)

extern const uint8_t server_root_cert_pem_start[] asm("_binary_server_root_cert_pem_start");
extern const uint8_t server_root_cert_pem_end[]   asm("_binary_server_root_cert_pem_end");

static const char* boundary = "12345678900000987654321";
static const char* boundary_header = "--12345678900000987654321";
static const char* boundary_tail = "\r\n--12345678900000987654321--";


char* form_data_create(const char* name, char* content)
{
	char *form_data_buf = NULL;
    if(asprintf(&form_data_buf, "%s\r\n"	\
    						"Content-Disposition: form-data; name=\"%s\"\r\n\r\n"	\
							"%s\r\n"	\
							, boundary_header, name, content) < 0){
    }
	return form_data_buf;
}


void form_data_free(char** form_data)
{	
	for(int i=0; form_data[i]!=NULL; i++){
		free(form_data[i]);
	}
}


esp_tls_t *http_tls_conn(url_t* url)
{
	if(strncmp(url->scheme, "https", strlen("https")) == 0){
		esp_tls_cfg_t cfg = {
            .cacert_pem_buf  = server_root_cert_pem_start,
            .cacert_pem_bytes = server_root_cert_pem_end - server_root_cert_pem_start,
        };
		return esp_tls_conn_new(url->host, strlen(url->host), url->port, &cfg);
	}else{
		return esp_tls_conn_new(url->host, strlen(url->host), url->port, NULL);
	}
}


void http_tls_free(esp_tls_t* tls)
{
	if(tls == NULL)
		return;

	esp_tls_conn_delete(tls); 
	tls = NULL;	
}


static esp_err_t http_tls_response(struct esp_tls *tls, http_parser_settings* http_callback)
{
    /* Read HTTP response */
    char recv_buf[256];
    bzero(recv_buf, sizeof(recv_buf));
    ssize_t read_bytes;

    http_parser parser;
    http_parser_init(&parser, HTTP_RESPONSE);
    ssize_t nparsed = 0;
	ssize_t timeout_cnt = 0;

	struct timeval tv = {HTTP_READ_TIMEOUT_S, 0};
    if (setsockopt(tls->sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
         HTTPC_DEBUGE(TAG, "... failed to set socket receiving timeout");
         return ESP_FAIL;
    }
    HTTPC_DEBUGI(TAG, "... set socket receiving timeout success");

    while(1) {	
		
		read_bytes = esp_tls_conn_read(tls, recv_buf, sizeof(recv_buf)-1);
        if(read_bytes == MBEDTLS_ERR_SSL_WANT_WRITE || read_bytes == MBEDTLS_ERR_SSL_WANT_READ)
            continue;
		else if(read_bytes == -0x4C || read_bytes == -1){
			timeout_cnt++;
			if(timeout_cnt < 10)
				continue;
			else{
				HTTPC_DEBUGE(TAG, "http_tls_read timeout");
				return ESP_FAIL;
			}
		}else if(read_bytes < 0){
			HTTPC_DEBUGE(TAG, "http_tls_read  returned -0x%x", -read_bytes);
    		return ESP_FAIL;
    	}else if(read_bytes == 0){
    		HTTPC_DEBUGI(TAG, "http connection closed");
    		break;
    	}

		timeout_cnt = 0;
        // using http parser causes stack overflow somtimes - disable for now
    	nparsed = http_parser_execute(&parser, http_callback, recv_buf, read_bytes);
    	if(nparsed != read_bytes){
    		HTTPC_DEBUGE(TAG, "http parser error: %s (%s)",
    	            http_errno_description(HTTP_PARSER_ERRNO(&parser)),
    	            http_errno_name(HTTP_PARSER_ERRNO(&parser)));
			return ESP_FAIL;
    	}

		if(ulTaskNotifyTake(pdTRUE, 0)){
        	break;
        }
    }

    HTTPC_DEBUGI(TAG, "... done reading from socket. Last read return=%d errno=%d", read_bytes, errno);
    return ESP_OK;
}


static esp_err_t http_tls_request(const char* uri, const char* method, const char* headers, const char* post_data, http_parser_settings* http_callback)
{
	url_t *url = url_parse(uri);
	if(url == NULL){
		HTTPC_DEBUGE(TAG, "Could not parse URI %s", uri);
		return ESP_FAIL;
	}

 	struct esp_tls *tls = http_tls_conn(url);		
    if(tls == NULL) {
        HTTPC_DEBUGE(TAG, "Could not connect to URI %s", uri);
		return ESP_FAIL;
    }

	esp_err_t err = ESP_FAIL;
    char *send_buf = NULL;
	//send <request line> and <header> and <blank line>
    if(asprintf(&send_buf, "%s %s HTTP/1.1\r\nHost: %s\r\n%s\r\n\r\n", method, url->path, url->host, headers) < 0){
    	HTTPC_DEBUGE(TAG, "send_buf malloc failed!");
        goto exit;
    }

	struct timeval tv = {HTTP_WRITE_TIMEOUT_S, 0};
    if (setsockopt(tls->sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
         HTTPC_DEBUGE(TAG, "... failed to set socket sending timeout");
         return ESP_FAIL;
    }
    HTTPC_DEBUGI(TAG, "... set socket sending timeout success");

	if (esp_tls_conn_write(tls, send_buf, strlen(send_buf)) < 0) {
	    free(send_buf);
		HTTPC_DEBUGE(TAG, "... http_hdr send failed");
		goto exit;
	}

    free(send_buf);
    HTTPC_DEBUGI(TAG, "... http_hdr send success");

	//send <request body>
	if(post_data != NULL){
		if (esp_tls_conn_write(tls, post_data, strlen(post_data)) < 0) {
			HTTPC_DEBUGE(TAG, "... http_body send failed");
			goto exit;
		}
		HTTPC_DEBUGI(TAG, "... http_body send success");
	}

	err = http_tls_response(tls, http_callback);

exit:
    url_free(url);
	esp_tls_conn_delete(tls); 
    return err;
}


esp_err_t http_client_post(const char* uri, const char* headers, const char* post_data, http_parser_settings* http_callback)
{
	return http_tls_request(uri, "POST", headers, post_data, http_callback);
}


esp_err_t http_client_get(const char* uri, const char* headers, http_parser_settings* http_callback)
{
	return http_tls_request(uri, "GET", headers, NULL, http_callback);
}


esp_err_t http_client_delete(const char* uri, const char* headers, const char* post_data, http_parser_settings* http_callback)
{
	return http_tls_request(uri, "DELETE", headers, post_data, http_callback);
}


esp_err_t http_client_put(const char* uri, const char* headers, const char* post_data, http_parser_settings* http_callback)
{
	return http_tls_request(uri, "PUT", headers, post_data, http_callback);
}


esp_err_t http_rest_post(url_t* url, struct esp_tls *tls, const char* headers, const char** form_data, Fat_Data_st* fat_data, http_parser_settings* http_callback)
{
	char *header_buf = NULL;
	char *multipart_buf = NULL;

    if(asprintf(&multipart_buf, "%s\r\n"	\
    							"Content-Disposition: form-data; name=\"file\"; filename=\"%s\"\r\n"	\
    							"Content-Type: %s\r\n\r\n"	\
								, boundary_header, fat_data->file_name, fat_data->file_type) < 0){
    	HTTPC_DEBUGE(TAG,"multipart_buf malloc failed!");
    	return ESP_FAIL;
    }

    size_t file_size = (fat_data->data_size) + (fat_data->hdr_size) + strlen(multipart_buf) + strlen(boundary_tail);	
	for(int i=0; form_data[i]!=NULL; i++){
		file_size += strlen(form_data[i]);
	}

    if(asprintf(&header_buf, "POST %s HTTP/1.1\r\nHost: %s\r\n%s\r\n"		\
    					   	 "Content-Type: multipart/form-data; boundary=%s\r\n"	\
							 "Content-Length: %d\r\n\r\n"				\
							 , url->path, url->host, headers, boundary, file_size) < 0){
    	HTTPC_DEBUGE(TAG, "header_buf malloc failed!");
		free(multipart_buf);
    	return ESP_FAIL;
    }

	struct timeval tv = {HTTP_WRITE_TIMEOUT_S, 0};
    if (setsockopt(tls->sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
         HTTPC_DEBUGE(TAG, "... failed to set socket sending timeout");
         return ESP_FAIL;
    }
    HTTPC_DEBUGI(TAG, "... set socket sending timeout success");

	//send <request line>, <header> and <blank line>
	if (esp_tls_conn_write(tls, header_buf, strlen(header_buf)) < 0) {
		HTTPC_DEBUGE(TAG, "... http_hdr send failed");
		free(multipart_buf);
	    free(header_buf);
		return ESP_FAIL;
	}

    free(header_buf);
    HTTPC_DEBUGI(TAG, "... http_hdr send success");

	for(int i=0; form_data[i]!=NULL; i++){
		if(esp_tls_conn_write(tls, (unsigned char *)form_data[i], strlen(form_data[i])) < 0){
			HTTPC_DEBUGE(TAG, "... form_data send failed");
			free(multipart_buf);
			return ESP_FAIL;
    	}
	}

	HTTPC_DEBUGI(TAG, "... form_data send success");

	//send <multipart_header>
	if (esp_tls_conn_write(tls, multipart_buf, strlen(multipart_buf)) < 0) {
		HTTPC_DEBUGE(TAG, "... multipart_hdr send failed");
		free(multipart_buf);
		return ESP_FAIL;
	}

    free(multipart_buf);
    HTTPC_DEBUGI(TAG, "... multipart_hdr send success");

	//send <fat_data_header>
    if((fat_data->hdr_data) != NULL){
    	if(esp_tls_conn_write(tls, fat_data->hdr_data, fat_data->hdr_size) < 0){
			HTTPC_DEBUGE(TAG, "... fat_data_hdr send failed");
        	return ESP_FAIL;
    	}
    }

    HTTPC_DEBUGI(TAG, "... fat_data_hdr send success");

	//send <fat_data_body>
	char *read_buf = malloc(2048);

	if(read_buf == NULL){
		HTTPC_DEBUGE(TAG, "read_buf malloc failed!");
		return ESP_FAIL;
	}

	for(ssize_t read_bytes = fat_data->src_offset; read_bytes < (fat_data->data_size); read_bytes += 2048){
		if(esp_partition_read(fat_data->esp_partition, read_bytes, read_buf, 2048) != ESP_OK){
	        HTTPC_DEBUGE(TAG, "... fat_data read failed");
			free(read_buf);
	        return ESP_FAIL;
		}

    	HTTPC_DEBUGI(TAG, "... fat_data read success");		

		if (esp_tls_conn_write(tls, read_buf, 2048) < 0) {
	        HTTPC_DEBUGE(TAG, "... fat_data send failed");
			free(read_buf);
	        return ESP_FAIL;
		}

    	HTTPC_DEBUGI(TAG, "... fat_data read send");			
	}

    HTTPC_DEBUGI(TAG, "... fat_data send success");

	//send <tail>
	if (esp_tls_conn_write(tls, boundary_tail, strlen(boundary_tail)) < 0) {
		HTTPC_DEBUGE(TAG, "... http_tail send failed");
		free(read_buf);
		return ESP_FAIL;
	}

    free(read_buf);
	HTTPC_DEBUGI(TAG, "... http_tail send success");

	return http_tls_response(tls, http_callback);
}