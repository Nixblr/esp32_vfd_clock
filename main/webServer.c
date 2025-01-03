

#include "webServer.h"

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_tls_crypto.h"
#include <esp_http_server.h>
#include "display.h"
#include <string.h>
#include "stdbool.h"

static const char *TAG = "webServer";

extern const char index_html_start[] asm("_binary_index_html_start");
extern const char index_html_end[] asm("_binary_index_html_end");
extern const char style_start[] asm("_binary_style_css_start");
extern const char style_end[] asm("_binary_style_css_end");
extern const char script_start[] asm("_binary_script_js_start");
extern const char script_end[] asm("_binary_script_js_end");

struct async_resp_arg
{
    httpd_handle_t hd;
    int fd;
    backendRequest_t requestData;
    bool updateAll;
};
static httpd_handle_t server = NULL;

//static esp_err_t trigger_async_send(httpd_handle_t handle, httpd_req_t *req);

/* Style */
static esp_err_t style_get_handler(httpd_req_t *req)
{
    char *buf;
    size_t buf_len;
    const size_t style_len = style_end - style_start;
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1)
    {
        buf = malloc(buf_len);
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK)
        {
            ESP_LOGI(TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, style_start, style_len);

    return ESP_OK;
}

/* Script */
static esp_err_t script_get_handler(httpd_req_t *req)
{
    char *buf;
    size_t buf_len;
    const size_t script_len = script_end - script_start;
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1)
    {
        buf = malloc(buf_len);
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK)
        {
            ESP_LOGI(TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, script_start, script_len);

    return ESP_OK;
}

/* An HTTP GET handler */
static esp_err_t index_get_handler(httpd_req_t *req)
{
    char *buf;
    size_t buf_len;
    const size_t index_html_len = index_html_end - index_html_start;
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1)
    {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK)
        {
            ESP_LOGI(TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }
    httpd_resp_send(req, index_html_start, index_html_len);
    return ESP_OK;
}

/*Dynamic data request*/
static esp_err_t sendDataHandler(httpd_req_t *req)
{
    const size_t index_html_len = index_html_end - index_html_start;
    char b2[100];
    b2[0] = 0;
    int ret, rem = req->content_len;
    char *param;
    int len;

    ESP_LOGI(TAG, "POST. Content len: %d", rem);
    while (rem > 0)
    {
        ret = httpd_req_recv(req, b2, MIN(rem, sizeof(b2)));
        if (ret < 0)
        {
            ESP_LOGE(TAG, "Request error %d", ret);
            if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            {
                continue;
            }
        }
        if (ret == 0)
        {
            ESP_LOGI(TAG, "Empty...");
            break;
        }
        rem -= ret;
    }
    char *buff = backendGetStateJSON(BR_FULL);
    size_t l = strlen(buff);
    httpd_resp_send(req, buff, l);
    free(buff);
    return ESP_OK;
}

/* Websocket data processing...*/
static esp_err_t handle_ws_req(httpd_req_t *req)
{
    if (req->method == HTTP_GET)
    {
        ESP_LOGI(TAG, "Handshake done, the new connection was opened");
        return ESP_OK;
    }

    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }

    if (ws_pkt.len)
    {
        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL)
        {
            ESP_LOGE(TAG, "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
            free(buf);
            return ret;
        }
        backendProcessData(ws_pkt.payload);
    }
    ESP_LOGI(TAG, "frame len is %d", ws_pkt.len);
    free(buf);
    return ESP_OK;
}

static void ws_async_send(void *arg)
{
    httpd_ws_frame_t ws_pkt;
    struct async_resp_arg *resp_arg = arg;

    static size_t max_clients = CONFIG_LWIP_MAX_LISTENING_TCP;
    size_t fds = max_clients;
    int client_fds[max_clients];
    int client_info;

    esp_err_t ret = httpd_get_client_list(server, &fds, client_fds);

    if (ret != ESP_OK)
    {
        free(resp_arg);
        return;
    }

    char *buff = backendGetStateJSON(resp_arg->requestData);
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = (uint8_t *)buff;
    ws_pkt.len = strlen(buff);
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    if (resp_arg->updateAll)
    {
        ESP_LOGI(TAG, "WS Multicast update. Requested: %d", (int) resp_arg->requestData);
        for (int i = 0; i < fds; i++)
        {
            client_info = httpd_ws_get_fd_info(server, client_fds[i]);
            if (client_info == HTTPD_WS_CLIENT_WEBSOCKET)
            {
                httpd_ws_send_frame_async(resp_arg->hd, client_fds[i], &ws_pkt);
            }
        }
    }
    else
    {
        ESP_LOGI(TAG, "WS  update.");
        client_info = httpd_ws_get_fd_info(server, client_fds[resp_arg->fd]);
        if (client_info == HTTPD_WS_CLIENT_WEBSOCKET)
        {
            ESP_LOGI(TAG, "WS  update to client.");
            httpd_ws_send_frame_async(resp_arg->hd, client_fds[resp_arg->fd], &ws_pkt);
        }
    }
    free(buff);
    free(resp_arg);
}

/*static esp_err_t trigger_async_send(httpd_handle_t handle, httpd_req_t *req)
{
    struct async_resp_arg *resp_arg = malloc(sizeof(struct async_resp_arg));
    resp_arg->hd = req->handle;
    resp_arg->fd = httpd_req_to_sockfd(req);
    resp_arg->updateAll = true;
    return httpd_queue_work(handle, ws_async_send, resp_arg);
}*/

esp_err_t webInterfaceUpdateToClients(backendRequest_t content)
{
    if (server == NULL)
    {
        return ESP_FAIL;
    }
    struct async_resp_arg *resp_arg = malloc(sizeof(struct async_resp_arg));
    resp_arg->hd = server;
    resp_arg->fd = 0;
    resp_arg->updateAll = true;
    resp_arg->requestData = content;
    return httpd_queue_work(server, ws_async_send, resp_arg);
}

static const httpd_uri_t style = {
    .uri = "/style.css",
    .method = HTTP_GET,
    .handler = style_get_handler,
    .user_ctx = NULL};
static const httpd_uri_t script = {
    .uri = "/script.js",
    .method = HTTP_GET,
    .handler = script_get_handler,
    .user_ctx = NULL};
static const httpd_uri_t indexReq = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = index_get_handler,
    .user_ctx = NULL};
static const httpd_uri_t sendDataReq = {
    .uri = "/index.html",
    .method = HTTP_POST,
    .handler = sendDataHandler,
    .user_ctx = NULL};
httpd_uri_t ws = {
    .uri = "/ws",
    .method = HTTP_GET,
    .handler = handle_ws_req,
    .user_ctx = NULL,
    .is_websocket = true};

esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    /* For any other URI send 404 and close socket */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &style);
        httpd_register_uri_handler(server, &script);
        httpd_register_uri_handler(server, &indexReq);
        httpd_register_uri_handler(server, &sendDataReq);
        httpd_register_uri_handler(server, &ws);
        return server;
    }
    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static void stop_webserver(httpd_handle_t server)
{
    httpd_stop(server);
}

static void disconnect_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    httpd_handle_t *server = (httpd_handle_t *)arg;
    if (*server)
    {
        ESP_LOGI(TAG, "Stopping webserver");
        stop_webserver(*server);
        *server = NULL;
    }
}

static void connect_handler(void *arg, esp_event_base_t event_base,
                            int32_t event_id, void *event_data)
{
    httpd_handle_t *server = (httpd_handle_t *)arg;
    if (*server == NULL)
    {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
}

void httpap_init(void)
{
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    //ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
    server = start_webserver();
}
