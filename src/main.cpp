#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <DHT.h>

#define LED_PIN 5
#define LDR_PIN 2
#define DHT_PIN 4
#define BTN_PIN 15
#define DHT_TYPE DHT11

#if CONFIG_FREERTOS_UNICORE
    static const BaseType_t app_cpu = 0;
#else
    static const BaseType_t app_cpu = 1;
#endif

// Estructura para almacenar datos de sensores
struct SensorData {
    float temperature;
    unsigned short int humidity;
    unsigned short int light;
};

void IRAM_ATTR button_ISR(){

}


// Tarea para alternar el estado del LED cada 4 segundos
void taskToggleLED(void *pvParameters) {
    void **params = (void **)pvParameters;
    SensorData *data = (SensorData *)params[0];
    SemaphoreHandle_t mutex = (SemaphoreHandle_t)params[1];
    
    while (true) {
        if (xSemaphoreTake(mutex, portMAX_DELAY)){
            if ( (data->temperature > 24 and data->humidity > 80) or (data->light > 500) ){
                digitalWrite( LED_PIN, !digitalRead ( LED_PIN ) );
            } else digitalWrite( LED_PIN, LOW );
            xSemaphoreGive(mutex);
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }
}

// Tarea para leer temperatura y humedad del sensor DHT11
void taskTemperature(void *pvParameters) {
    void **params = (void **)pvParameters;
    SensorData *data = (SensorData *)params[0];
    DHT* dht = (DHT *)params[1];
    SemaphoreHandle_t mutex = (SemaphoreHandle_t)params[2];
    float temperature = 0.0f;

    while (true) {
        if (xSemaphoreTake(mutex, portMAX_DELAY)) {
            temperature = dht->readTemperature();
            
            if (!isnan(temperature)) {
                data->temperature = temperature;
            } else {
                // Usar el promedio de 3 temperaturas anteriores
                data->temperature = -1.0f;
            }

            xSemaphoreGive(mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void taskHumidity(void *pvParameters){
    void **params = (void **)pvParameters;
    SensorData *data = (SensorData *)params[0];
    DHT* dht = (DHT *)params[1];
    SemaphoreHandle_t mutex = (SemaphoreHandle_t)params[2];
    unsigned short int humidity = 0;

    while (true) {
        if (xSemaphoreTake(mutex, portMAX_DELAY)) {
            humidity = dht->readHumidity();
            // Dividir en dos tareas
            if (!isnan(humidity)) {
                data->humidity = humidity;
            } else {
                // Usar el promedio de 3 temperaturas anteriores
                data->humidity = 0;
            }

            xSemaphoreGive(mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// Tarea para leer el sensor LDR y almacenar el valor en la estructura compartida
void taskLight(void *pvParameters) {
    void **params = (void **)pvParameters;
    SensorData *data = (SensorData *)params[0];
    SemaphoreHandle_t mutex = (SemaphoreHandle_t)params[1];

    unsigned short int light = 0;

    while (true) {
        
        if (xSemaphoreTake(mutex, portMAX_DELAY)) {
            light = analogRead(LDR_PIN); 
            data->light = light;
            xSemaphoreGive(mutex);
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// Tarea para imprimir los datos de los sensores en el puerto serie
void taskPrintData(void *pvParameters) {
    void **params = (void **)pvParameters;
    SensorData *data = (SensorData *)params[0];
    SemaphoreHandle_t mutex = (SemaphoreHandle_t)params[1];

    while (true) {
        if (xSemaphoreTake(mutex, portMAX_DELAY)) {
            // Cambiar por un arreglo
            Serial.printf   ("------------\nTemperatura: %.2f\nHumedad: %d\nLuz: %d\n------------\n",
                            data->temperature, data->humidity, data->light);
            xSemaphoreGive(mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(BTN_PIN, INPUT_PULLDOWN);
    pinMode(LED_PIN, OUTPUT);
    pinMode(LDR_PIN, INPUT);
    
    // Interrupciones
    attachInterrupt(BTN_PIN, button_ISR, CHANGE);

    // DHT
    static DHT dht(DHT_PIN, DHT_TYPE);
    dht.begin();

    // Crear el mutex
    SemaphoreHandle_t mutex = xSemaphoreCreateMutex();

    // **¡IMPORTANTE!** Declaramos `data` como `static` para que no sea destruida al salir de `setup()`
    static SensorData data = {0.0f, 0, 0};

    // Parámetros para las tareas
    void *paramsDHT[3] = {&data, &dht, mutex};
    void *paramData[2] = {&data, mutex};

    // Creación de tareas con prioridades adecuadas
    xTaskCreate(taskToggleLED, "Toggle LED", 2048, paramData, 3, NULL);
    xTaskCreate(taskHumidity, "Read Humidity Function", 2048, paramsDHT, 2, NULL);
    xTaskCreate(taskTemperature, "Read Temperature Function", 2048, paramsDHT, 2, NULL);
    xTaskCreate(taskLight, "Light Function", 2048, paramData, 2, NULL);
    xTaskCreate(taskPrintData, "Print Data", 2048, paramData, 1, NULL);
}

void loop() {}
