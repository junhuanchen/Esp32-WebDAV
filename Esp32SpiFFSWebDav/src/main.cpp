
#include <arduino.h>

#include <WiFi.h>

const char *ssid = "tgoffice";
const char *password = "tu9u2017";

// const char *ssid = "zhiwu";
// const char *password = "zhiwu123";

void WiFiEvent(WiFiEvent_t event)
{
    Serial.printf("[WiFi-event] event: %d\n", event);

    switch (event)
    {
    case SYSTEM_EVENT_STA_GOT_IP:
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        Serial.println("WiFi lost connection");
        break;
    }
}

void wifi_init()
{
    // delete old config
    WiFi.disconnect(true);

    delay(1000);

    WiFi.mode(WIFI_AP_STA);
    
    WiFi.mode(WIFI_AP);
    
    IPAddress apIP(192, 168, 4, 1);

    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

    WiFi.softAP("uno32.bpi");

    WiFi.onEvent(WiFiEvent);

    WiFi.begin(ssid, password);

    Serial.println("Wait for WiFi... ");
}

#include "mongoose.h"

#define MG_PORT_HTTP "80"
#define MG_PORT_DNS "udp://:53"

struct mg_serve_http_opts opts;

static void mg_ev_http_handler(struct mg_connection *nc, int ev, void *p)
{
    static const char *reply_fmt =
        "HTTP/1.0 200 OK\r\n"
        "Connection: close\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "Hello %s\n";

    switch (ev)
    {
    case MG_EV_ACCEPT:
    {
        char addr[32];
        mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr), MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
        printf("Connection %p from %s\n", nc, addr);
        break;
    }
    case MG_EV_HTTP_REQUEST:
    {
        char addr[32];
        struct http_message *hm = (struct http_message *)p;
        mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr), MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
        printf("HTTP request from %s: %.*s %.*s\n", addr, (int)hm->method.len,
               hm->method.p, (int)hm->uri.len, hm->uri.p);

        // mg_printf(nc, reply_fmt, addr);

        mg_serve_http(nc, hm, opts);

        break;
    }
    case MG_EV_CLOSE:
    {
        printf("Connection %p closed\n", nc);
        break;
    }
    case MG_EV_POLL:
        break;
    case MG_EV_RECV:
        break;
    case MG_EV_SEND:
    {
        char addr[32];
        struct http_message *hm = (struct http_message *)p;
        mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr), MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
        printf("HTTP request from %s: %.*s %.*s\n", addr, (int)hm->method.len,
               hm->method.p, (int)hm->uri.len, hm->uri.p);
        // mg_printf(nc, reply_fmt, addr);
        // nc->flags |= MG_F_SEND_AND_CLOSE;
        break;
    }
    default:
        printf("UnKnown %p result : %d\n", nc, ev);
        break;
    }
}

static void mg_ev_dns_handler(struct mg_connection *nc, int ev, void *ev_data)
{
    static char name[512];
    static in_addr_t s_our_ip_addr = inet_addr("192.168.4.1");

    switch (ev)
    {
        case MG_DNS_MESSAGE:
        {
            struct mg_dns_message *msg = (struct mg_dns_message *)ev_data;
            struct mg_dns_reply reply = mg_dns_create_reply(&nc->send_mbuf, msg);

            for (int i = 0; i < msg->num_questions; i++)
            {
                struct mg_dns_resource_record *rr = &msg->questions[i];
                if (rr->rtype == MG_DNS_A_RECORD)
                {
                    mg_dns_uncompress_name(msg, &rr->name, name, sizeof(name) - 1);

                    if (strcmp(name, "uno32.bpi") == 0)
                    {
                        mg_dns_reply_record(&reply, rr, NULL, rr->rtype, 3600, &s_our_ip_addr, 4);
                    }
                    else if (strcmp(name, "www.uno32.bpi") == 0)
                    {
                        mg_dns_reply_record(&reply, rr, NULL, MG_DNS_CNAME_RECORD, 3600, "uno32.bpi", strlen("uno32.bpi"));

                        mg_dns_reply_record(&reply, rr, "uno32.bpi", rr->rtype, 3600, &s_our_ip_addr, 4);
                    }
                }
            }
            mg_dns_send_reply(nc, &reply);
            nc->flags |= MG_F_SEND_AND_CLOSE;
            break;
        }
    default:
        break;
    }
}

struct mg_mgr mgr_http;
struct mg_mgr mgr_dns;

void mg_init()
{
    // webdev
    opts.document_root = "/spiffs";
    opts.dav_document_root = "/spiffs";
    opts.dav_auth_file = "-";

    printf("Starting web-server on port %s\n", MG_PORT_HTTP);
    
    mg_mgr_init(&mgr_http, NULL);

    struct mg_connection *nc_http = mg_bind(&mgr_http, MG_PORT_HTTP, mg_ev_http_handler);

    mg_set_protocol_http_websocket(nc_http);

    if (nc_http == NULL)
    {
        printf("Error setting up listener!\n");
        return;
    }

    printf("Starting dns-server on port %s\n", MG_PORT_DNS);

    mg_mgr_init(&mgr_dns, NULL);

    mg_set_nameserver(&mgr_dns, "bpi.uno32");

    struct mg_connection *nc_dns = mg_bind(&mgr_dns, MG_PORT_DNS, mg_ev_dns_handler);

    mg_set_protocol_dns(nc_dns);

    if (nc_dns == NULL)
    {
        printf("Error setting up listener!\n");
        return;
    }
    
}

void mg_poll()
{
    mg_mgr_poll(&mgr_dns, 10);
    mg_mgr_poll(&mgr_http, 100);
}

#include "SPIFFS.h"

#include "soc/rtc.h"

void setup()
{
    Serial.begin(115200);
    if (!SPIFFS.begin())
    {
        if (!SPIFFS.format())
            Serial.println("SPIFFS Format Failed");
        if (!SPIFFS.begin())
        {
            Serial.println("SPIFFS Mount Failed");
            return;
        }
    }

    // return ;

    rtc_clk_cpu_freq_set(RTC_CPU_FREQ_80M);
    
    wifi_init();
    mg_init();
}

void loop()
{
    // return ;
    // if (WiFi.softAPgetStationNum())// if(WiFi.isConnected())
    {
        // Serial.println("TaskBackground Running");
        mg_poll();
    }
}
