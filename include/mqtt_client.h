#include <mqtt/connect_options.h>
#include <mqtt/client.h>
#include <MQTTClient.h>
#include <mqtt/buffer_ref.h>

/* - REPLACE - Adjust parameters as you want */
#define ADDRESS "localhost:1883"
#define CLIENTID_PUB "pub-system"
#define CLIENTID_SUB "sub-system"

const mqtt::string_ref user_name("omid"); // MQTT username, not DB
const mqtt::buffer_ref<char> password("12345678"); // MQTT pass, not DB