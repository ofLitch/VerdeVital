#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <DHT.h>

#define LED_PIN 5
#define LDR_PIN 2
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
    int light;
};

void taskToggleLED(void *pvParameters) {
    while (true) {
        //Serial.println("LED Apagado");
        digitalWrite(LED_PIN, LOW);
        vTaskDelay(pdMS_TO_TICKS(4000));

        //Serial.println("LED Encendido");
        digitalWrite(LED_PIN, HIGH);   
        vTaskDelay(pdMS_TO_TICKS(4000));

        UBaseType_t stackRemaining = uxTaskGetStackHighWaterMark(NULL);
        Serial.print("Stack restante en taskLED: ");
        Serial.println(stackRemaining);

    }
}

void taskDHT(void *pvParameters) {
    void **params = (void **)pvParameters;
    SensorData *data = (SensorData *)params[0];
    DHT* dht = (DHT *)params[1];
    SemaphoreHandle_t mutex = (SemaphoreHandle_t)params[2];
    float temperature = 0.0f;
    float humidity = 0.0f;

    while (true) {
        if ( xSemaphoreTake(mutex, portMAX_DELAY) ){
            temperature = dht->readTemperature();
            humidity = dht->readHumidity();
            
            if (!isnan(temperature) && !isnan(humidity)) {
                data->temperature = temperature;
                data->humidity = humidity;
            } else {
                data->temperature = -1.0f;
                data->humidity = -1.0f;
            }
            UBaseType_t stackRemaining = uxTaskGetStackHighWaterMark(NULL);
            Serial.print("Stack restante en taskDHT: ");
            Serial.println(stackRemaining);

            xSemaphoreGive(mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void taskLight(void *pvParameters){
    void **params = (void **)pvParameters;
    SensorData *data = (SensorData *)params[0];
    SemaphoreHandle_t mutex = (SemaphoreHandle_t)params[1];
    

    while (true){
        if ( xSemaphoreTake(mutex, portMAX_DELAY) ){
            int light = analogRead(LDR_PIN);
            data->light = light;

            xSemaphoreGive(mutex);
            UBaseType_t stackRemaining = uxTaskGetStackHighWaterMark(NULL);
            Serial.print("Stack restante en taskLight: ");
            Serial.println(stackRemaining);

        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void taskPrintData(void *pvParameters){
    void **params = (void **)pvParameters;
    SensorData *data = (SensorData *)params[0];
    SemaphoreHandle_t mutex = (SemaphoreHandle_t)params[1];

    while (true){
        if ( xSemaphoreTake(mutex, portMAX_DELAY) ){
            Serial.println("------------");
            Serial.printf   ("------------\nTemperatura: %.2f\nHumedad: %.2f\nLuz: %.2f\n------------\n",
                            data->temperature, data->humidity, data->light);

            UBaseType_t stackRemaining = uxTaskGetStackHighWaterMark(NULL);
            Serial.print("Stack restante en taskPrintData: ");
            Serial.println(stackRemaining);

            xSemaphoreGive(mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(LDR_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);

    static DHT dht(DHT_PIN, DHT_TYPE);
    dht.begin();

    // Crear el mutex
    SemaphoreHandle_t mutex = xSemaphoreCreateMutex();

    // Estructura  de datos
    SensorData data = {0.0f, 0.0f, 0};

    // Parámetros 
    void *paramsDHT[3] = {&data, &dht, mutex};
    void *paramData[2] = {&data, mutex};

    // Tareas
    xTaskCreate(taskToggleLED, "Toggle LED", 2048, NULL, 3, NULL);
    xTaskCreate(taskDHT, "DHT Humidity & Temperature Function", 2048, paramsDHT, 2, NULL);
    xTaskCreate(taskLight, "Light Function", 2048, paramData, 2, NULL);
    xTaskCreate(taskPrintData, "Printing Data", 3072, paramData, 1, NULL);
}

void loop() {}