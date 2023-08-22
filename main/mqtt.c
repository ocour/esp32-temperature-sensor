#include "mqtt_client.h"
#include "esp_log.h"
#include "esp_system.h"
#include "mqtt.h"
#include "main.h"
#include "my_nvs.h"
#include "ble_prov_gatt.h" 

// THINGNAME
static char thing_name[AWS_THING_NAME_MAX_SIZE];

// PEM certificates
static char server_cert[SERVER_CERT_MAX_SIZE];
static char client_cert[CLIENT_CERT_MAX_SIZE];
static char client_key[CLIENT_KEY_SIZE];

// CERTIFICATES GOTTEN FROM CreateKeysAndCertificate MQTT API CALL's response
static char certificate_id[CERTIFICATE_ID_SIZE];
static char certificate_pem[CERTIFICATE_PEM_SIZE];
static char private_key[PRIVATE_KEY_SIZE];
static char certificate_ownership_token[CERTIFICATE_OWNERSHIP_TOKEN];

// tmp_buf will be used for string replacement
static char tmp_buf[TMPBUFFER_SIZE];

// json buffer
static char json_buffer[CREATE_KEYS_AND_CERT_RESPONSE_SIZE];

static void mqtt_start(esp_event_handler_t event_handler, char *clientId);
static void con_mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
static void claim_mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

/**
 *  Publish RegisterThing MQTT API call
*/
static int register_thing(esp_mqtt_client_handle_t client, const char *thing_name, const char *certificate_ownership_token);

/**
 * Parse response of CreateKeysAndCertificate MQTT API call
*/
static int parse_create_keys_and_certificates_response(
    const char* json_buffer, char *certificate_id, char *certificate_pem,
     char *private_key, char *certificate_ownership_token
);

/// @brief Get string from json and format it to PEM format 
/// @param json, json object that will be parsed
/// @param key, json key of value 
/// @param out_value, buffer to store parsed value
/// @param size_of_out_value
/// @return 0 for success; -1 for failure
static int json_parse_and_format_pem(const char *json, const char *key, char *out_value, int size_of_out_value);

/// Get string from json
static int json_parse(const char *json, const char *key, char *out_value);

/**
 *  Get value of key from json object
*/
static int json_get_string(const char *json, const char *key, char *value);

/**
 *  Credit to jmucchiello from stackoverflow
 *  https://stackoverflow.com/questions/779875/what-function-is-to-replace-a-substring-from-a-string-in-c
 *  I edited it a bit to eliminate the dynamic allocation
*/
static int str_replace(char *dest, int size_of_dest, char *orig, char *rep, char *with);

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

esp_err_t mqtt_get_tls_certificates(
    const char *server_cert_nvs_key, const char *client_cert_nvs_key, const char *client_key_nvs_key
)
{
    return nvs_get_tls_certs(
        server_cert, server_cert_nvs_key,
        client_cert, client_cert_nvs_key,
        client_key, client_key_nvs_key
    );
}

void mqtt_register_thing(void)
{
    // GET CLAIM CERTS
    esp_err_t err = nvs_get_tls_certs(
        server_cert, NVS_KEY_SERVER_CERT,
        client_cert, NVS_KEY_CLAIM_CLIENT_CERT,
        client_key, NVS_KEY_CLAIM_CLIENT_KEY
    );
    if(err != ESP_OK) {
        printf("Error getting claim certs.\n");
        return;
    }

    // start MQTT to register thing
    mqtt_start(claim_mqtt_event_handler, CLAIM_THINGNAME);
}

void mqtt_start_sending_data(void)
{
    esp_err_t err;

    // if any are empty, get them from nvs storage
    if(server_cert[0] == '\0' || client_cert[0] == '\0' || client_key[0] == '\0') {
        // GET CONNECTION CERTS
        err = nvs_get_tls_certs(
            server_cert, NVS_KEY_SERVER_CERT,
            client_cert, NVS_KEY_CON_CLIENT_CERT,
            client_key, NVS_KEY_CON_CLIENT_KEY
        );
        if(err != ESP_OK) {
            printf("Error getting connection certs.\n");
            return;
        }
    }

    err = nvs_get_thing_name(thing_name);
    if(err != ESP_OK) {
        printf("Error getting thingname.\n");
        return;
    }

    // start MQTT to send temperature data
    mqtt_start(con_mqtt_event_handler, thing_name);
}

static void mqtt_start(esp_event_handler_t event_handler, char *clientId)
{
    // printf("MQTT_URL=%s\n", MQTT_URL);

    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_URL,
        .broker.verification.certificate = (const char *)server_cert,
        .credentials = {
            .client_id = clientId,
            .authentication = {
                .certificate = (const char *)client_cert,
                .key = (const char *)client_key,
            },
        }
    };

    ESP_LOGI(TAG, "[APP] Free memory: %ld bytes", esp_get_free_heap_size());
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, event_handler, NULL);
    esp_mqtt_client_start(client);
}

/*
 * @brief MQTT event handler for sending temperature data
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void con_mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%ld", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

        /// TODO: CHANGE TO SEND TEMPERATURE DATA
        // PUBLISH TEST
        msg_id = esp_mqtt_client_publish(client, "example/topic", "{}", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);

        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");

        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);

        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

/*
 * @brief MQTT event handler for registering thing
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void claim_mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%ld", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    int ret;
    int json_buffer_len;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

        /// TODO: CHANGE QOS TO 1 AND ACCOUNT FOR DUPLICATES

        // SUBSCRIBE TO TOPICS
        // CreateKeysAndCertificate MQTT API
        msg_id = esp_mqtt_client_subscribe(client, TOPIC_CREATE_KEYS_AND_CERT_ACCEPTED, 0);
        ESP_LOGI(TAG, "Subscribed to %s topic, msg_id=%d", TOPIC_CREATE_KEYS_AND_CERT_ACCEPTED, msg_id);

        msg_id = esp_mqtt_client_subscribe(client, TOPIC_CREATE_KEYS_AND_CERT_REJECTED, 0);
        ESP_LOGI(TAG, "Subscribed to %s topic, msg_id=%d", TOPIC_CREATE_KEYS_AND_CERT_REJECTED, msg_id);

        // RegisterThing MQTT API
        msg_id = esp_mqtt_client_subscribe(client, TOPIC_REGISTER_THING_ACCEPTED, 0);
        ESP_LOGI(TAG, "Subscribed to %s topic, msg_id=%d", TOPIC_REGISTER_THING_ACCEPTED, msg_id);

        msg_id = esp_mqtt_client_subscribe(client, TOPIC_REGISTER_THING_REJECTED, 0);
        ESP_LOGI(TAG, "Subscribed to %s topic, msg_id=%d", TOPIC_REGISTER_THING_REJECTED, msg_id);

        // PUBLISH CreateKeysAndCertificate MQTT API with empty payload
        msg_id = esp_mqtt_client_publish(client, TOPIC_CREATE_KEYS_AND_CERT, "{}", 0, 0, 0);
        ESP_LOGI(TAG, "Sent publish successful to %s topic, msg_id=%d", TOPIC_CREATE_KEYS_AND_CERT, msg_id);

        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);

        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");

        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);

        // CreateKeysAndCertificate MQTT API call successful
        if(strncmp(TOPIC_CREATE_KEYS_AND_CERT_ACCEPTED, event->topic, event->topic_len) == 0) {

            // Gather fragmented response to buffer
            strncpy(json_buffer+event->current_data_offset, event->data, event->data_len);

            json_buffer_len = strlen(json_buffer);
            // When response has been gathered parse it
            if(json_buffer_len >= event->total_data_len) {

                // parse response
                ret = parse_create_keys_and_certificates_response(
                    json_buffer, certificate_id, certificate_pem, private_key, certificate_ownership_token
                );
                if(ret != 0) {
                    printf("Function parse_create_keys_and_certificates_response() failed.\n");
                    return;
                }

                // GET THINGNAME not from NVS
                ret = nvs_get_thing_name(thing_name);
                if(ret != ESP_OK) {
                    printf("Error getting thingname.\n");
                    return;
                }

                // RegisterThing MQTT API call
                ret = register_thing(client, thing_name, certificate_ownership_token);
                if(ret != ESP_OK) {
                    printf("Function create_thing() failed.\n");
                    return;
                } else {
                    printf("Successfully RegisteredThing!\n");
                }
            }
        } else if(strncmp(TOPIC_REGISTER_THING_ACCEPTED, event->topic, event->topic_len) == 0) {
            // RegisterThing MQTT API call successful

            // SAVE CERTIFICATES TO NVS STORAGE
            ret = nvs_set_tls_certs(
                certificate_pem, NVS_KEY_CON_CLIENT_CERT,
                private_key, NVS_KEY_CON_CLIENT_KEY
            );
            if(ret != ESP_OK) {
                printf("Error setting connection certs.\n");
                return;
            }

            /// TODO: REBOOT?
            esp_restart();
        } else {
            // CreateKeysAndCertificate or RegisterThing failed
            printf("DATA=%.*s\r\n", event->data_len, event->data);
        }

        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static int register_thing(esp_mqtt_client_handle_t client, const char *thing_name, const char *certificate_ownership_token)
{
    char payload[REGISTER_THING_PAYLOAD_SIZE];
    int ret;

    // Format payload
    ret = snprintf(payload, REGISTER_THING_PAYLOAD_SIZE,
        "{\"%s\":\"%s\", \"parameters\":{\"ThingName\":\"%s\"}}", 
        JSON_KEY_CERTIFICATE_OWNERSHIP_TOKEN, certificate_ownership_token, thing_name);
    
    if(ret < 0) {
        printf("Failed to create payload\n");
        return -1;
    }
    // printf("payload=%s\n", payload);

    // RegisterThing MQTT API call
    int msg_id = esp_mqtt_client_publish(client, TOPIC_REGISTER_THING, payload, 0, 0, 0);
    ESP_LOGI(TAG, "Sent publish successful to %s topic, msg_id=%d", TOPIC_REGISTER_THING, msg_id);
    return 0;
}

static int parse_create_keys_and_certificates_response(
    const char* json_buffer, char *certificate_id, char *certificate_pem, char *private_key, char *certificate_ownership_token
)
{
    int ret;

    // parse certificateId
    ret = json_parse(json_buffer, JSON_KEY_CERTIFICATE_ID, certificate_id);
    if(ret != 0) {
        return ret;
    }

    // parse certificatePem
    ret = json_parse_and_format_pem(json_buffer, JSON_KEY_CERTIFICATE_PEM, certificate_pem, CERTIFICATE_PEM_SIZE);
    if(ret != 0) {
        return ret;
    }

    // parse privateKey
    ret = json_parse_and_format_pem(json_buffer, JSON_KEY_PRIVATE_KEY, private_key, PRIVATE_KEY_SIZE);
    if(ret != 0) {
        return ret;
    }

    // parse certificateOwnershipToken
    ret = json_parse(json_buffer, JSON_KEY_CERTIFICATE_OWNERSHIP_TOKEN, certificate_ownership_token);
    if(ret != 0) {
        return ret;
    }

    return 0;
}

static int json_parse_and_format_pem(const char *json, const char *key, char *out_value, int size_of_out_value)
{
    int ret;
    char *tmp;

    ret = json_parse(json, key, tmp_buf);
    if(ret != 0) {
        return ret;
    }

    // Replace \\n with \n for correct PEM format
    printf("Formatting %s json string to PEM... ", key);
    ret = str_replace(out_value, size_of_out_value, tmp_buf, "\\n", "\n");
    if(ret != 0) {
        printf("Failed to perform str_replace()\n");
        return ret;
    }

    // replace last newline ('\n') with \0, assuming it exists
    tmp = strrchr(out_value, '\n');
    *tmp = '\0';

    printf("Done.\n");
    return 0;
}

static int json_parse(const char *json, const char *key, char *out_value)
{
    int ret;
    printf("Parsing %s... ", key);

    ret = json_get_string(json, key, out_value);
    if(ret != 0) {
        printf("Failed to parse %s.\n", key);
        return -1;
    }

    printf("Done.\n");
    return ret;
}

static int json_get_string(const char *json, const char *key, char *value)
{
    char *pch;
    int len;
    // GET KEY IN JSON
    pch = strstr(json, key);

    // IF KEY EXISTS IN json
    if(pch != NULL) {
        // Locate end of key
        pch = strchr(pch, ':');

        // Locate start of value
        pch = strchr(pch, '\"');
        // printf("%s\n", pch);
        
        // Locate end of value
        len = strcspn(pch+1, "\"");
        // printf("value=%.*s\n", len, pch+1);

        // +1 to not include the "
        strncpy(value, pch+1, len);
        value[len] = '\0';
        return 0;
    }
    else {
        return -1;
    }
}

static int str_replace(char *dest, int size_of_dest, char *orig, char *rep, char *with)
{
    char result[STR_REPLACE_BUFFER_SIZE];   // the return string
    char *ins;                              // the next insert point
    char *tmp;                              // varies
    int len_rep;                            // length of rep (the string to remove)
    int len_with;                           // length of with (the string to replace rep with)
    int len_front;                          // distance between rep and end of last rep
    int count;                              // number of replacements

    // sanity checks and initialization
    if (!orig || !rep)
        return -1;
    len_rep = strlen(rep);
    if (len_rep == 0)
        return -1; // empty rep causes infinite loop during count
    if (!with)
        with = "";
    len_with = strlen(with);

    // count the number of replacements needed
    ins = orig;
    for (count = 0; (tmp = strstr(ins, rep)); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result;

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    
    strncpy(dest, result, size_of_dest);
    
    return 0;
}