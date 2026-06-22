# Sistema de Cama de Inmersión Automatizada (CIA)

Proyecto desarrollado para la materia **Diseño e implementación de sistemas mecatrónicos**.  
El sistema automatiza el proceso de riego por inmersión en una cama de aclimatación vegetal, integrando sensores, actuadores, control con ESP32 e interfaz web local.

## Descripción general

El sistema permite controlar una cama de inmersión utilizada para el riego por capilaridad de bandejas de germinación.  
Su objetivo es reducir la intervención manual, mejorar la repetibilidad del proceso, monitorear humedad del sustrato y recircular el agua utilizada durante el ciclo.

El proyecto integra:

- Lectura de sensores de humedad del sustrato.
- Lectura de sensores de nivel de agua en cama y depósito.
- Monitoreo de temperatura y humedad ambiental con DHT22.
- Control de bomba de llenado.
- Control de bomba de drenado.
- Control de válvula solenoide.
- Control de ventiladores AC.
- Modo manual.
- Modo automático.
- Interfaz web local mediante ESP32 y LittleFS.

## Tecnologías utilizadas

- ESP32
- PlatformIO
- Framework Arduino
- C++
- HTML
- CSS
- JavaScript
- LittleFS
- DHT22
- Relevadores activos en LOW

## Estructura del proyecto

```text
DISM-CIA/
├── data/
│   ├── icons/
│   ├── index.html
│   ├── script.js
│   └── style.css
├── include/
├── lib/
├── src/
│   ├── Actuators.cpp
│   ├── Actuators.h
│   ├── Config.h
│   ├── IOManager.cpp
│   ├── IOManager.h
│   ├── Sensors.cpp
│   ├── Sensors.h
│   ├── StateMachine.cpp
│   ├── StateMachine.h
│   ├── WebServerManager.cpp
│   ├── WebServerManager.h
│   └── main.cpp
├── test/
├── platformio.ini
└── README.md
