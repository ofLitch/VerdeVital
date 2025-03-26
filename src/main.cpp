/**
 * @file main.cpp
 * @brief Sistema embebido para el monitoreo y control ambiental en un invernadero de cannabis medicinal.
 * 
 * @details Este programa utiliza RTOS para realizar el monitoreo y control ambiental dentro de un invernadero.
 *          Incluye lectura de sensores, control de actuadores, registro de datos, y soporte para alarmas visuales y sonoras.
 * 
 * @author
 *          Valentina Muñoz Arcos
 *          Luis Miguel Gómez Muñoz
 *          David Alejandro Ortega Flórez
 * 
 * @version 1.2
 * @date 2025-03-26
 */

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <DHT.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>

// Directivas
#define LED_PIN   5        ///< Pin para el LED
#define BUZZER_PIN 25      ///< Pin para el buzzer
#define LDR_PIN   2        ///< Pin para el sensor LDR
#define DHT_PIN   4        ///< Pin para el sensor DHT
#define BTN_PIN   15       ///< Pin para el botón
#define IO_PIN    27       ///< Pin para DAT (DS1302)
#define SCLK_PIN  14       ///< Pin para CLK (DS1302)
#define CE_PIN    26       ///< Pin para RST (DS1302)
#define IR_PIN    16       ///< Pin para el sensor infrarrojo ARD SO
#define DHT_TYPE  DHT11    ///< Tipo de sensor DHT usado

/**
 * @brief Instancias de clases para el módulo RTC DS1302
 */
ThreeWire myWire(IO_PIN, SCLK_PIN, CE_PIN); ///< Configuración de pines del RTC DS1302
RtcDS1302<ThreeWire> Rtc(myWire);          ///< Instancia para la comunicación con el RTC DS1302

#if CONFIG_FREERTOS_UNICORE
    static const BaseType_t app_cpu = 0; ///< Configuración para sistemas de un núcleo
#else
    static const BaseType_t app_cpu = 1; ///< Configuración para sistemas de múltiples núcleos
#endif

/**
 * @struct SensorData
 * @brief Estructura para almacenar datos de los sensores ambientales.
 * 
 * @details Esta estructura contiene los valores medidos de temperatura, humedad, luz y la fecha/hora actual.
 */
struct SensorData {
    float temperature;        ///< Temperatura medida por el sensor DHT
    unsigned short humidity;  ///< Humedad medida por el sensor DHT
    unsigned short light;     ///< Nivel de luz medido por el sensor LDR
    RtcDateTime dateTime;     ///< Hora leída por el DS1302
};

/**
 * @brief Alterna el estado del LED basado en condiciones ambientales.
 * 
 * @param pvParameters Puntero a los parámetros que incluye datos compartidos y mutex.
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
 * @brief Activa una alarma sonora mediante un buzzer si las condiciones exceden los límites.
 * 
 * @param pvParameters Puntero a los parámetros que incluye datos compartidos y mutex.
 */
void taskBuzzerAlarm(void *pvParameters) {
    void **params = (void **)pvParameters;
    SensorData *data = (SensorData *)params[0];
    SemaphoreHandle_t mutex = (SemaphoreHandle_t)params[1];

    while (true) {
        if (xSemaphoreTake(mutex, portMAX_DELAY)) {
            // Activar el buzzer si se superan los límites
            if (data->temperature > 24 && data->humidity > 80 || data->light > 500) {
                for (int i = 0; i < 3; i++) {  // Patrón sonoro
                    digitalWrite(BUZZER_PIN, HIGH);
                    vTaskDelay(pdMS_TO_TICKS(200));
                    digitalWrite(BUZZER_PIN, LOW);
                    vTaskDelay(pdMS_TO_TICKS(200));
                }
            }
            xSemaphoreGive(mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(3000));  // Frecuencia de verificación
    }
}


/**
 * @brief Lee la temperatura desde el sensor DHT y actualiza los datos compartidos.
 * 
 * @param pvParameters Puntero a los parámetros que incluye datos compartidos, sensor y mutex.
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
 * @brief Lee la humedad desde el sensor DHT y actualiza los datos compartidos.
 * 
 * @param pvParameters Puntero a los parámetros que incluye datos compartidos, sensor y mutex.
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
 * @brief Lee el nivel de luz desde el sensor LDR y actualiza los datos compartidos.
 * 
 * @param pvParameters Puntero a los parámetros que incluye datos compartidos y mutex.
 */
void taskLight(void *pvParameters) {
    void **params = (void **)pvParameters;
    SensorData *data = (SensorData *)params[0];
    SemaphoreHandle_t mutex = (SemaphoreHandle_t)params[1];

    while (true) {
        
        if (xSemaphoreTake(mutex, portMAX_DELAY)) {
            data->light = analogRead(LDR_PIN);
            xSemaphoreGive(mutex);
        }

        vTaskDelay(pdMS_TO_TICKS(2400));
    }
}

/**
 * @brief Lee la fecha y hora del módulo RTC DS1302.
 * 
 * @param pvParameters Puntero a los parámetros que incluye datos compartidos y mutex.
 */
void taskClockTime(void *pvParameters) {
    void **params = (void **)pvParameters;
    SensorData *data = (SensorData *)params[0];
    SemaphoreHandle_t mutex = (SemaphoreHandle_t)params[1];

    RtcDateTime now = Rtc.GetDateTime();

    while (true) {
        
        if (xSemaphoreTake(mutex, portMAX_DELAY)) {
            now = Rtc.GetDateTime();
            data-> dateTime = now;
            xSemaphoreGive(mutex);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief Imprime los datos de los sensores en el puerto serial.
 * 
 * @param pvParameters Puntero a los parámetros que incluye datos compartidos y mutex.
 */
 void taskPrintData(void *pvParameters) {
    void **params = (void **)pvParameters;
    SensorData *data = (SensorData *)params[0];
    SemaphoreHandle_t mutex = (SemaphoreHandle_t)params[1];

    while (true) {
        char buffer[128]; // Arreglo para manejar cadenas optimizado
        if (xSemaphoreTake(mutex, portMAX_DELAY)) {
            snprintf(buffer, sizeof(buffer), 
                "------------\nTemperatura: %.2f°C\nHumedad: %d%%\nLuz: %d\nFecha: %04d-%02d-%02d %02d:%02d:%02d\n------------\n",
                data->temperature, data->humidity, data->light, 
                data->dateTime.Year(), data->dateTime.Month(), data->dateTime.Day(),
                data->dateTime.Hour(), data->dateTime.Minute(), data->dateTime.Second());

            Serial.println(buffer);
            xSemaphoreGive(mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(2500));  // Ajuste de prioridad para mantener eficiencia
    }
}

/**
 * @brief Obtiene la fecha y hora de compilación para inicializar el RTC DS1302.
 * 
 * @return RtcDateTime La fecha y hora de compilación.
 */
RtcDateTime getCompileDateTime() {
    const char monthNames[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    
    char monthStr[4];
    int day, year, hour, minute, second;
    sscanf(__DATE__, "%s %d %d", monthStr, &day, &year);
    sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second);

    int month = (strstr(monthNames, monthStr) - monthNames) / 3 + 1;

    return RtcDateTime(year, month, day, hour, minute, second);
}

/**
 * @brief Configuración inicial del sistema que incluye inicialización de sensores, actuadores y tareas de FreeRTOS.
 */
void setup() {
    // Inicializar y asignación de valores
    Serial.begin(115200);
    pinMode(BTN_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    pinMode(LDR_PIN, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    //RTC
    Rtc.Begin();

    if (Rtc.GetIsWriteProtected()) {
        Serial.println("Quitando protección contra escritura...");
        Rtc.SetIsWriteProtected(false);
    }

    if (!Rtc.GetIsRunning()) {
        Serial.println("El RTC estaba detenido. Iniciando...");
        Rtc.SetIsRunning(true);
    }

    Rtc.SetDateTime(getCompileDateTime());
    
    // DHT
    static DHT dht(DHT_PIN, DHT_TYPE);
    dht.begin();
    
    // Crear el mutex
    SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
    
    // Declaramos `data` como `static` para que no sea destruida al salir de `setup()`
    static SensorData data = {0.0f, 0, 0, Rtc.GetDateTime()};

    // Declarar variables locales en el setup
    static unsigned int counter = 0;                      // Contador local
    static unsigned long buttomLastDebounceTime = 0;      // Último tiempo de RISING del botón para debounce
    static unsigned long infraredLastDebounceTime = 0;    // último tiempo de RISING del s. infrarrojo para debouce

    // Lambda para gestionar el botón (interrupción)
    attachInterrupt(BTN_PIN, [&counter, &buttomLastDebounceTime]() {
        unsigned long currentTime = millis();
        if (currentTime - buttomLastDebounceTime > 200) {
            counter++;
            buttomLastDebounceTime = currentTime;
            Serial.print("Counter actualizado: ");
            Serial.println(counter);
        }
    }, RISING);

    attachInterrupt(IR_PIN, [&counter, &infraredLastDebounceTime]() {
        unsigned long currentTime = millis();
        if (currentTime - infraredLastDebounceTime > 600) {
            counter++;
            infraredLastDebounceTime = currentTime;
            Serial.print("Counter actualizado: ");
            Serial.println(counter);
        }
    }, RISING);

    // Parámetros para las tareas
    void *paramsDHT[3] = {&data, &dht, mutex};
    void *paramData[2] = {&data, mutex};

    digitalWrite(LED_PIN, LOW);

    // Creación de tareas con prioridades adecuadas
    xTaskCreate(taskToggleLED, "Toggle LED", 2048, paramData, 3, NULL);
    xTaskCreate(taskBuzzerAlarm, "Buzzer Alarm", 2048, paramData, 3, NULL);
    xTaskCreate(taskHumidity, "Read Humidity Function", 2048, paramsDHT, 2, NULL);
    xTaskCreate(taskTemperature, "Read Temperature Function", 2048, paramsDHT, 2, NULL);
    xTaskCreate(taskLight, "Light Function", 2048, paramData, 2, NULL);
    xTaskCreate(taskPrintData, "Print Data", 2048, paramData, 1, NULL);
    xTaskCreate(taskClockTime, "Read Time", 2048, paramData, 3, NULL);    
}

/**
 * @brief Bucle principal del programa.
 * 
 * @details Este bucle permanece vacío porque todas las operaciones son manejadas por las tareas de FreeRTOS.
 */
void loop() {}