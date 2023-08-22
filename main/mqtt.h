#pragma once

#include <stdio.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Used as ClientId when connecting to register thing
#define CLAIM_THINGNAME CONFIG_MQTT_CLAIM_CLIENT_ID

// AWS ENDPOINT
#define MQTT_URL    "mqtts://" CONFIG_MQTT_ENDPOINT ":" CONFIG_MQTT_PORT

// TOPICS
#define TOPIC_CREATE_KEYS_AND_CERT            "$aws/certificates/create/json"
#define TOPIC_CREATE_KEYS_AND_CERT_ACCEPTED   "$aws/certificates/create/json/accepted"
#define TOPIC_CREATE_KEYS_AND_CERT_REJECTED   "$aws/certificates/create/json/rejected"
#define TOPIC_REGISTER_THING                  "$aws/provisioning-templates/" CONFIG_AWS_TEMPLATE_NAME "/provision/json"
#define TOPIC_REGISTER_THING_ACCEPTED         "$aws/provisioning-templates/" CONFIG_AWS_TEMPLATE_NAME "/provision/json/accepted"
#define TOPIC_REGISTER_THING_REJECTED         "$aws/provisioning-templates/" CONFIG_AWS_TEMPLATE_NAME "/provision/json/rejected"

// CERTIFICATES FOR TLS
#define SERVER_CERT_MAX_SIZE    4096
#define CLIENT_CERT_MAX_SIZE    4096
#define CLIENT_KEY_SIZE         4096

// CERTIFICATES GOTTEN FROM CreateKeysAndCertificate MQTT API CALL's response
#define CERTIFICATE_ID_SIZE         64
#define CERTIFICATE_PEM_SIZE        CLIENT_CERT_MAX_SIZE
#define PRIVATE_KEY_SIZE            CLIENT_KEY_SIZE
#define CERTIFICATE_OWNERSHIP_TOKEN 1024

// tmpBuf will be used for string replacement
#define TMPBUFFER_SIZE  CERTIFICATE_PEM_SIZE

/* 
    This buffer will hold the responce of CreateKeysAndCertificate
    Since there is a limit to how much data a mqtt (or wifi) message can contain,
    the response is cut to smaller pieces (fragments), this buffer will be used to store the
    combined message. 
 */
#define CREATE_KEYS_AND_CERT_RESPONSE_SIZE  4096

// Json keys of CreateKeysAndCertificate MQTT API response
#define JSON_KEY_CERTIFICATE_ID                 "certificateId"
#define JSON_KEY_CERTIFICATE_PEM                "certificatePem"
#define JSON_KEY_PRIVATE_KEY                    "privateKey"
#define JSON_KEY_CERTIFICATE_OWNERSHIP_TOKEN    "certificateOwnershipToken"

// The payload size of the RegisterThing MQTT API call
#define REGISTER_THING_PAYLOAD_SIZE 2048

// buffer size of the str_replace function, it needs to be large enough to hold a pem certificate
#define STR_REPLACE_BUFFER_SIZE CLIENT_CERT_MAX_SIZE

/**
 *  Used to get tls certificates from nvs storage
*/
esp_err_t mqtt_get_tls_certificates(
    const char *server_cert_nvs_key, const char *client_cert_nvs_key, const char *client_key_nvs_key
);

/**
 *  Start MQTT to register thing
*/
void mqtt_register_thing(void);

/**
 * Start mqtt to send temperature data
*/
void mqtt_start_sending_data(void);

#ifdef __cplusplus
}
#endif