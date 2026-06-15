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

Módulos principales
main.cpp

Inicializa los módulos principales del sistema y ejecuta el ciclo de operación.

Config.h

Contiene la configuración general de pines, umbrales, tiempos máximos e intervalos de lectura.

Sensors

Gestiona la lectura de sensores de humedad, sensores de nivel y sensor DHT22.

Actuators

Controla bomba de llenado, bomba de drenado, válvula solenoide y ventiladores.

IOManager

Administra entradas digitales, botones físicos, sensores de nivel y salidas hacia relevadores.

StateMachine

Implementa la lógica de operación en modo manual y modo automático mediante una máquina de estados.

WebServerManager

Configura el servidor web local, el punto de acceso WiFi, LittleFS y la comunicación con la interfaz web.

Modos de operación
Modo manual

Permite al operador activar funciones específicas del sistema, como:

Llenado.
Drenado.
Ventilación.

El modo manual se utiliza para pruebas, mantenimiento, diagnóstico o intervención directa.

Modo automático

Ejecuta la secuencia completa del sistema:

Monitoreo.
Llenado.
Riego por capilaridad.
Drenado.
Ventilación.
Retorno a monitoreo.
Interfaz web local

El ESP32 crea un punto de acceso WiFi y sirve una interfaz web almacenada en LittleFS.
Desde esta interfaz se puede:

Visualizar humedad promedio del sustrato.
Ver lecturas individuales de sensores.
Consultar temperatura y humedad ambiental.
Ver estado de nivel de cama y depósito.
Revisar estado de bombas, válvula y ventiladores.
Cambiar entre modo manual y automático.
Iniciar modo automático.
Enviar comandos manuales.
Activar paro web.
Reiniciar el sistema.
Configuración WiFi

El sistema genera una red local desde el ESP32.

SSID: ESP32-CIA
Password: 12345678
Actuadores controlados
Bomba de llenado.
Bomba de drenado.
Válvula solenoide.
Ventiladores AC.

Los relevadores trabajan con lógica activa en LOW.

Sensores utilizados
4 sensores de humedad del sustrato.
2 sensores de nivel en cama.
2 sensores de nivel en depósito.
1 sensor DHT22 para temperatura y humedad ambiental.
Instalación y carga
Abrir el proyecto en Visual Studio Code con PlatformIO.
Conectar el ESP32 por USB.
Compilar el proyecto.
Cargar el firmware al ESP32.
Cargar el sistema de archivos LittleFS.
Conectarse a la red WiFi generada por el ESP32.
Abrir la dirección IP del ESP32 en el navegador.
Dependencias

Las dependencias se encuentran configuradas en platformio.ini:

DHT sensor library
Adafruit Unified Sensor
Equipo
Cristofer Delgado Urdiera
Jorge Luis Alvarez Martínez
Sanghun You
Edgar Chávez Pineda
Luis Felipe Lara Cabrera
Maximiliano Escobar Mireles
