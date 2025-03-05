#include "StateMachineLib.h"
#include "AsyncTaskLib.h"
#include <DHT.h>

// Definición de Pines
#define PIN_LED 13
#define DHTPIN 14
#define LDR_PIN 27
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// Declaración de funciones y tareas
void Timeout(void);
AsyncTask TimeoutTask(5000, false, Timeout);

void readTempFunct(void);
AsyncTask readTempTask(2500, true, readTempFunct);

void readHumFunct(void);
AsyncTask readHumTask(3200, true, readHumFunct);

void readLuzFunct(void);
AsyncTask readLuzTask(1600, true, readLuzFunct);

void ledONFunct(void);
AsyncTask ledONTask(700, true, ledONFunct);

void ledOFFFunct(void);
AsyncTask ledOFFTask(500, true, ledOFFFunct);

// State Alias
enum State
{
    monitAmb = 0,
    monitLuz = 1,
    alarma = 2
};

// Input Alias
enum Input
{
    sign_T = 0,
    sign_L = 1,
    sign_H = 2,
    Unknown = 3,
};

// Create new StateMachine
StateMachine stateMachine(3, 5);

// Stores last user input
Input input;

// Declaración de variables globales
float temperatura = 0.0;
float humedad = 0.0;

// Declaración de funciones OnEntering y OnLeaving
void Funct_AMB_Init(void);
void Funct_AMB_fin(void);
void Funct_LUZ_Init(void);
void Funct_LUZ_fin(void);
void Funct_ALARMA_Init(void);
void Funct_ALARMA_fin(void);

// Setup the State Machine
void setupStateMachine()
{
    // Add transitions
    stateMachine.AddTransition(monitAmb, monitLuz, []() { return input == sign_T; });
    stateMachine.AddTransition(monitAmb, alarma, []() { return input == sign_H; });

    stateMachine.AddTransition(monitLuz, monitAmb, []() { return input == sign_T; });
    stateMachine.AddTransition(monitLuz, alarma, []() { return input == sign_L; });

    stateMachine.AddTransition(alarma, monitAmb, []() { return input == sign_T; });

    // Add actions
    stateMachine.SetOnEntering(monitAmb, Funct_AMB_Init);
    stateMachine.SetOnEntering(monitLuz, Funct_LUZ_Init);
    stateMachine.SetOnEntering(alarma, Funct_ALARMA_Init);

    stateMachine.SetOnLeaving(monitAmb, Funct_AMB_fin);
    stateMachine.SetOnLeaving(monitLuz, Funct_LUZ_fin);
    stateMachine.SetOnLeaving(alarma, Funct_ALARMA_fin);
}

// Configuración inicial
void setup()
{
    Serial.begin(9600);
    Serial.println("Iniciando la máquina de estados...");
    dht.begin();                  // Inicialización del sensor DHT
    pinMode(PIN_LED, OUTPUT);     // Configura el LED como salida
    pinMode(LDR_PIN, INPUT);      // Configura el LDR como entrada
    setupStateMachine();          // Configuración de la máquina de estados
    Serial.println("Máquina iniciada.");

    // Estado inicial
    stateMachine.SetState(monitAmb, false, true);
}

// Bucle principal
void loop()
{
    TimeoutTask.Update();
    readTempTask.Update();
    readHumTask.Update();
    readLuzTask.Update();

    // Eliminar dependencias cruzadas
    ledONTask.Update();
    ledOFFTask.Update();

    stateMachine.Update();
    input = Unknown;
}


// Función Timeout
void Timeout(void)
{
    input = sign_T;
}

// Función para lectura de temperatura
void readTempFunct(void)
{
    temperatura = dht.readTemperature();
    if (isnan(temperatura))
    {
        Serial.println("Error leyendo temperatura.");
        return;
    }
    Serial.print("Temperatura: ");
    Serial.println(temperatura);
    if ((temperatura > 24) && (humedad > 70))
    {
        input = sign_H;
    }
}

// Función para lectura de humedad
void readHumFunct(void)
{
    humedad = dht.readHumidity();
    if (isnan(humedad))
    {
        Serial.println("Error leyendo humedad.");
        return;
    }
    Serial.print("Humedad: ");
    Serial.println(humedad);
    if ((temperatura > 24) && (humedad > 70))
    {
        input = sign_H;
    }
}

// Función para lectura de luz
void readLuzFunct(void)
{
    int luz = analogRead(LDR_PIN);
    Serial.print("Nivel de luz: ");
    Serial.println(luz);
    if (luz > 500)
    {
        input = sign_L;
    }
}

// Funciones para acciones del estado
void Funct_AMB_Init(void)
{
    Serial.println("Función AMB Init llamada.");
    TimeoutTask.SetIntervalMillis(5000); 
    TimeoutTask.Start();
    readTempTask.Start();
    readHumTask.Start();
}

void Funct_AMB_fin(void)
{
    Serial.println("Función AMB Fin llamada.");
    readTempTask.Stop();
    readHumTask.Stop();
    TimeoutTask.Stop();
}

void Funct_LUZ_Init(void)
{
    Serial.println("Función LUZ Init llamada.");
    TimeoutTask.SetIntervalMillis(3000);
    readLuzTask.Start();
    TimeoutTask.Start();
}

void Funct_LUZ_fin(void)
{
    Serial.println("Función LUZ Fin llamada.");
    readLuzTask.Stop();
    TimeoutTask.Stop();
}

void Funct_ALARMA_Init(void)
{
    Serial.println("Función ALARMA Init llamada.");
    TimeoutTask.SetIntervalMillis(6000);
    TimeoutTask.Start();
    ledONTask.Start();
    ledOFFTask.Start();
    digitalWrite(PIN_LED, HIGH);  // Fuerza el encendido para comprobar si el LED funciona
}


void ledONFunct(void)
{
    Serial.println("LED Encendido."); // Mensaje para depuración
    digitalWrite(PIN_LED, HIGH); // Enciende el LED
}

void ledOFFFunct(void)
{
    Serial.println("LED Apagado."); // Mensaje para depuración
    digitalWrite(PIN_LED, LOW); // Apaga el LED
}

void Funct_ALARMA_fin(void)
{
    Serial.println("Función ALARMA Fin llamada.");
    TimeoutTask.Stop();
    ledONTask.Stop();
    ledOFFTask.Stop();
    digitalWrite(PIN_LED, LOW); // Asegúrate de apagar el LED al salir del estado
}
