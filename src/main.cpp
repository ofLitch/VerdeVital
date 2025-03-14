#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <DHT.h>

#define LED_PIN LED_BUILTIN
#define DHT_PIN 4
#define DHT_TYPE DHT11

#if CONFIG_FREERTOS_UNICORE
    static const BaseType_t app_cpu = 0;
#else
    static const BaseType_t app_cpu = 1;
#endif

// Estructura para almacenar datos de sensores y RTC
struct SensorData {
    float temperature;
    float humidity;
};

void taskToggleLED(void *pvParameters) {
    while (true) {
        Serial.println("LED Apagado");
        digitalWrite(LED_PIN, LOW);
        vTaskDelay(pdMS_TO_TICKS(4000));

        Serial.println("LED Encendido");
        digitalWrite(LED_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(4000));
    }
}

void taskTemperature(void *pvParameters) {
    void **params = (void **)pvParameters;
    SensorData *data = (SensorData *)params[0];
    DHT* dht = (DHT *)params[1];
    while (true) {
        data->temperature = dht->readTemperature();
        Serial.print("Temperatura: ");
        Serial.println(data->temperature);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void taskHumidity(void *pvParameters) {
    void **params = (void **)pvParameters;
    SensorData *data = (SensorData *)params[0];
    DHT* dht = (DHT *)params[1];

    while (true) {
        data->humidity = dht->readHumidity();
        Serial.print("Humedad: ");
        Serial.println(data->humidity);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);

    static DHT dht(DHT_PIN, DHT_TYPE);
    dht.begin();

    // Estructura  de datos
    SensorData data = {0.0, 0.0};

    // Parámetros 
    void *paramsTemperature[2] = {&data, &dht};
    void *paramsHumidity[2] = {&data, &dht};

    // Tareas
    xTaskCreate(taskToggleLED, "Toggle LED", 1024, NULL, 1, NULL);
    xTaskCreate(taskTemperature, "Temperature Function", 2048, paramsTemperature, 1, NULL);
    xTaskCreate(taskHumidity, "Humidity Function", 2048, paramsHumidity, 2, NULL);
}

void loop() {}
