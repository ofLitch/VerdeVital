/**
 * @file main.cpp
 * @brief Sistema embebido para el monitoreo y control ambiental en un invernadero de cannabis medicinal.
 * 
 * @details Este programa gestiona un invernadero mediante sensores y actuadores que optimizan las condiciones
 *          de cultivo. Incluye captura de datos de temperatura, humedad relativa y luz ambiental, control de 
 *          un LED RGB, y monitoreo remoto mediante puerto serie.
 * 
 * @authors
 *          Valentina Muñoz Arcos
 *          Luis Miguel Gómez Muñoz
 *          David Alejandro Ortega Flórez
 * 
 * @version 1.2
 * @date 2025-03-26
 */

// Librerias
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <DHT.h>

// Directivas
#define LED_PIN 5       ///< Pin para el LED
#define LDR_PIN 2       ///< Pin para el sensor LDR
#define DHT_PIN 4       ///< Pin para el sensor DHT
#define BTN_PIN 15      ///< Pin para el botón
#define DHT_TYPE DHT11  ///< Tipo de sensor DHT usado

#if CONFIG_FREERTOS_UNICORE
    static const BaseType_t app_cpu = 0; ///< Configuración para sistemas de un núcleo
#else
    static const BaseType_t app_cpu = 1; ///< Configuración para sistemas de múltiples núcleos
#endif

/**
  * @struct SensorData
  * @brief Estructura para almacenar datos de sensores.
  */
struct SensorData {
    float temperature;        ///< Temperatura medida por el sensor DHT
    unsigned short humidity;  ///< Humedad medida por el sensor DHT
    unsigned short light;     ///< Nivel de luz medido por el sensor LDR
};

/**
  * @brief Tarea para alternar el estado del LED.
  * 
  * @param pvParameters Parámetros de la tarea: datos compartidos y mutex.
  */
void taskToggleLED(void *pvParameters) {
    void **params = (void **)pvParameters;
    SensorData *data = (SensorData *)params[0];
    SemaphoreHandle_t mutex = (SemaphoreHandle_t)params[1];

    while (true) {
        if (xSemaphoreTake(mutex, portMAX_DELAY)) {
            if ((data->temperature > 24 && data->humidity > 80) || (data->light > 500)) 
                digitalWrite(LED_PIN, !digitalRead(LED_PIN));
            else 
                digitalWrite(LED_PIN, LOW);
            
            xSemaphoreGive(mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(2100));  // Ajuste de prioridad: Tarea más espaciada en tiempo
    }
}

/**
  * @brief Tarea para leer la temperatura del sensor DHT.
  * 
  * @param pvParameters Parámetros de la tarea: datos compartidos, sensor y mutex.
 */
void taskTemperature(void *pvParameters) {
    void **params = (void **)pvParameters;
    SensorData *data = (SensorData *)params[0];
    DHT *dht = (DHT *)params[1];
    SemaphoreHandle_t mutex = (SemaphoreHandle_t)params[2];

    while (true) {
        float temperature = dht->readTemperature();
        if (!isnan(temperature)) {
            if (xSemaphoreTake(mutex, portMAX_DELAY)) {
                data->temperature = temperature;
                xSemaphoreGive(mutex);
            }
        } else Serial.println("Error leyendo temperatura. Usando valor anterior.");
        
        vTaskDelay(pdMS_TO_TICKS(2200));  // Ajuste de prioridad para frecuencia de lectura
    }
}

/**
  * @brief Tarea para leer la humedad del sensor DHT.
  * 
  * @param pvParameters Parámetros de la tarea: datos compartidos, sensor y mutex.
 */
void taskHumidity(void *pvParameters) {
    void **params = (void **)pvParameters;
    SensorData *data = (SensorData *)params[0];
    DHT *dht = (DHT *)params[1];
    SemaphoreHandle_t mutex = (SemaphoreHandle_t)params[2];

    while (true) {
        float humidity = dht->readHumidity();
        if (!isnan(humidity)) {
            if (xSemaphoreTake(mutex, portMAX_DELAY)) {
                data->humidity = (unsigned short)humidity;
                xSemaphoreGive(mutex);
            }
        } else Serial.println("Error leyendo humedad. Usando valor anterior");
        
         vTaskDelay(pdMS_TO_TICKS(2300));  // Ajuste de prioridad para frecuencia de lectura
    }
}



/**
 * @brief Tarea para leer el nivel de luz del sensor LDR.
 * 
 * @param pvParameters Puntero a los parámetros de la tarea. Incluye datos compartidos y mutex.
 */
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

        vTaskDelay(pdMS_TO_TICKS(2400));
    }
}

/**
  * @brief Tarea para imprimir los datos de los sensores.
  * 
  * @param pvParameters Parámetros de la tarea: datos compartidos y mutex.
  */
 void taskPrintData(void *pvParameters) {
    void **params = (void **)pvParameters;
    SensorData *data = (SensorData *)params[0];
    SemaphoreHandle_t mutex = (SemaphoreHandle_t)params[1];

    while (true) {
        char buffer[128]; // Arreglo para manejar cadenas optimizado
        if (xSemaphoreTake(mutex, portMAX_DELAY)) {
            snprintf(buffer, sizeof(buffer), 
                        "------------\nTemperatura: %.2f\nHumedad: %d\nLuz: %d\n------------\n",
                        data->temperature, data->humidity, data->light);
            Serial.println(buffer);
            xSemaphoreGive(mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(2500));  // Ajuste de prioridad para mantener eficiencia
    }
}

/**
 * @brief Configuración inicial del sistema para usar FreeRTOS.
 */
void setup() {
    // Inicializar y asignación de valores
    Serial.begin(115200);
    pinMode(BTN_PIN, INPUT_PULLDOWN);
    pinMode(LED_PIN, OUTPUT);
    pinMode(LDR_PIN, INPUT);
    
    // DHT
    static DHT dht(DHT_PIN, DHT_TYPE);
    dht.begin();
    
    // Crear el mutex
    SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
    
    // Declaramos `data` como `static` para que no sea destruida al salir de `setup()`
    static SensorData data = {0.0f, 0, 0};

    // Declarar variables locales en el setup
    static unsigned int counter = 0;          // Contador local
    static unsigned long lastDebounceTime = 0; // Último tiempo de pulsación para debounce

    // Lambda para gestionar el botón (interrupción)
    attachInterrupt(BTN_PIN, [&counter, &lastDebounceTime]() {
        unsigned long currentTime = millis();
        if (currentTime - lastDebounceTime > 200) {
            counter++;
            lastDebounceTime = currentTime;
            Serial.print("Counter actualizado: ");
            Serial.println(counter);
        }
    }, RISING);

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

/**
 * @brief Bucle principal del programa.
 * 
 * @details Este bucle permanece vacío porque todas las operaciones son manejadas por las tareas de FreeRTOS.
 */
void loop() {}