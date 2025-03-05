# README - Sistema de Monitoreo y Alarma para Invernadero VerdeVital

## Descripción del Proyecto
Este proyecto implementa un sistema de monitoreo ambiental y de luz para el invernadero **VerdeVital**, utilizando una **ESP32** y sensores de temperatura, humedad y luz. Además, el sistema cuenta con una máquina de estados para gestionar los diferentes estados del invernadero y un sistema de alarma basado en un LED intermitente.

## Pruebas (Fotos)
Se encuentran dentro de la carpeta "/img/firts/"

## Entorno de Desarrollo
- **Plataforma utilizada:** Visual Studio Code con PlatformIO.
- **Microcontrolador:** ESP32.
- **Lenguaje de programación:** C++.

## Hardware Utilizado
### Microcontrolador
- **ESP32**

### Sensores y Actuadores
- **DHT11** - Sensor de temperatura y humedad.
- **LDR** - Sensor de luz ambiente.
- **LED** - Indicador de alarma.

### Conexiones
| Componente | Pin ESP32 |
|------------|----------|
| DHT11 (Datos) | GPIO14 |
| LDR (Salida analógica) | GPIO27 |
| LED | GPIO13 |

## Estructura del Código
El código implementa una **máquina de estados finitos (FSM)** y tareas asíncronas para monitorear y gestionar el invernadero.

### Estados de la Máquina de Estados
1. **monitAmb (Monitoreo de Ambiente)**:
   - Se encarga de leer la temperatura y humedad del ambiente.
2. **monitLuz (Monitoreo de Luz)**:
   - Mide el nivel de luz y detecta si es necesario cambiar de estado.
3. **alarma (Alarma Activada)**:
   - Activa el LED intermitente en caso de condiciones críticas (alta temperatura y humedad o exceso de luz).

### Fragmento del Código Principal
```cpp
// Definición de Pines
#define PIN_LED 13
#define DHTPIN 14
#define LDR_PIN 27

// Estado inicial de la máquina de estados
void setup()
{
    Serial.begin(9600);
    Serial.println("Iniciando la máquina de estados...");
    dht.begin();
    pinMode(PIN_LED, OUTPUT);
    pinMode(LDR_PIN, INPUT);
    setupStateMachine();
    Serial.println("Máquina iniciada.");
    stateMachine.SetState(monitAmb, false, true);
}
```

## Funcionamiento
1. **Lectura de Sensores**:
   - Cada sensor se lee en intervalos de tiempo específicos usando `AsyncTask`.
2. **Máquina de Estados**:
   - Cambia de estado basado en las condiciones ambientales detectadas.
3. **Sistema de Alarma**:
   - Activa un LED intermitente cuando se detectan condiciones de riesgo.

## Instalación y Configuración en PlatformIO
1. Instalar PlatformIO en VS Code.
2. Clonar el repositorio del proyecto.
3. Configurar el entorno para **ESP32** en `platformio.ini`.
4. Compilar y subir el código a la ESP32.

## Contribuciones
Este proyecto es parte del desarrollo del invernadero VerdeVital. Se aceptan mejoras y optimizaciones. ¡Colabora con nosotros!

---

*Autor: Equipo de Desarrollo VerdeVital*
Valentina Muñoz Arcos
Luis Miguel Gomez Muñoz
David Alejandro Ortega Flórez

