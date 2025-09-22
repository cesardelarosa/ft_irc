# 🗺️ Roadmap de Desarrollo `ft_irc`

Este documento detalla los pasos recomendados para desarrollar el servidor IRC, desde la base hasta la implementación completa de las funcionalidades exigidas y su robustez.

## Fase 1: Cimientos y Lógica de Comandos

**Objetivo:** Crear una base sólida para el manejo y procesamiento de comandos, y la gestión del estado del cliente.

### 1.1. Refactorización del Manejo de Datos (`_handleClientData`)
* **Qué hacer:** Modificar la función `_handleClientData` para que no solo lea, sino que acumule datos en un buffer asociado a cada `Client`. Debe ser capaz de detectar y extraer comandos completos terminados en `\r\n`.
* **Ficheros a modificar:** `Server.cpp`, `Client.hpp`, `Client.cpp`.
* **Por qué:** Es el requisito más fundamental. Sin una forma fiable de recibir comandos completos, nada más puede funcionar. Esto soluciona el problema de los "mensajes parciales".

### 1.2. Implementación del `CommandParser`
* **Qué hacer:** Crear una clase o un conjunto de funciones, posiblemente dentro de `CommandHandler`, que tome un string (el comando completo) y lo divida en sus componentes: prefijo (opcional), comando y parámetros.
* **Ficheros a modificar:** `CommandHandler.cpp`, `CommandHandler.hpp`.
* **Por qué:** Separa la lógica de "parseo" de la lógica de "ejecución", haciendo el código mucho más limpio y fácil de mantener.

### 1.3. Sistema de Despacho de Comandos (`Command Dispatcher`)
* **Qué hacer:** Dentro del `CommandHandler`, implementar un sistema (por ejemplo, un `std::map<std::string, void (CommandHandler::*)(...)>`) que asocie cada comando (`NICK`, `JOIN`, etc.) con la función que debe ejecutarlo.
* **Ficheros a modificar:** `CommandHandler.cpp`, `CommandHandler.hpp`.
* **Por qué:** Evita tener un `if-else` gigante y permite añadir nuevos comandos de forma muy sencilla.

### 1.4. Sistema de Respuestas al Cliente
* **Qué hacer:** Crear una función centralizada para enviar respuestas a los clientes. IRC tiene respuestas numéricas estandarizadas (ej: `RPL_WELCOME`, `ERR_NICKNAMEINUSE`). Esta función debería facilitar el envío de esos mensajes formateados.
* **Ficheros a modificar:** `Server.cpp`, `Server.hpp` (o una nueva clase `ServerReplies.cpp`).
* **Por qué:** Centraliza toda la comunicación de salida, garantizando un formato consistente y facilitando la depuración.

---

## Fase 2: Funcionalidad Mínima Viable (Conexión y Chat Básico)

**Objetivo:** Lograr que un cliente IRC de referencia pueda conectarse, registrarse y enviar mensajes en un canal.

### 2.1. Secuencia de Autenticación y Registro
* **Qué hacer:** Implementar los primeros comandos que un cliente envía:
    1.  `PASS`: Comprobar la contraseña del servidor.
    2.  `NICK`: Asignar un nickname al usuario. Gestionar colisiones (nicknames ya en uso).
    3.  `USER`: Asignar el username, hostname, etc.
* **Ficheros a modificar:** `CommandHandler.cpp`, `Client.cpp`, `Client.hpp`.
* **Por qué:** Es el primer bloque funcional indispensable. Un cliente debe poder registrarse para hacer cualquier otra cosa.

### 2.2. Gestión de Canales (Básico)
* **Qué hacer:**
    * Implementar el comando `JOIN`. Si el canal no existe, se crea. El primer usuario en un canal es operador.
    * El `Server` necesitará un contenedor para los canales (`std::map<std::string, Channel*>`).
    * La clase `Channel` necesitará un contenedor para sus miembros (`std::vector<Client*>`).
* **Ficheros a modificar:** `CommandHandler.cpp`, `Channel.cpp`, `Channel.hpp`, `Server.cpp`, `Server.hpp`.
* **Por qué:** El chat en canales es la funcionalidad principal de IRC.

### 2.3. Envío de Mensajes
* **Qué hacer:**
    * Implementar `PRIVMSG`. Debe funcionar tanto para enviar mensajes a un canal (se reenvía a todos los miembros) como para enviar mensajes privados a otro usuario.
* **Ficheros a modificar:** `CommandHandler.cpp`.
* **Por qué:** Con esto se completa el ciclo de "chat" básico, que es el corazón del proyecto.

---

## Fase 3: Robustez y Funcionalidades de Operador

**Objetivo:** Completar todos los requisitos mandatorios del `subject` y asegurar que el servidor maneja todos los casos de error de forma estable.

### 3.1. Comandos de Operador de Canal
* **Qué hacer:** Implementar la lógica de operadores de canal y los comandos asociados:
    * `KICK`: Expulsar a un usuario de un canal.
    * `INVITE`: Invitar a un usuario a un canal (especialmente útil si es de solo invitación).
    * `TOPIC`: Cambiar o ver el topic de un canal.
* **Ficheros a modificar:** `CommandHandler.cpp`, `Channel.cpp`, `Channel.hpp`.
* **Por qué:** Son requisitos obligatorios y añaden la capa de gestión de canales.

### 3.2. Modos de Canal (`MODE`)
* **Qué hacer:** Implementar el comando `MODE` con todos sus flags:
    * `i`: Canal de solo invitación.
    * `t`: Topic solo modificable por operadores.
    * `k`: Poner/quitar contraseña (key) al canal.
    * `o`: Dar/quitar privilegios de operador a un usuario.
    * `l`: Poner/quitar límite de usuarios en el canal.
* **Ficheros a modificar:** `CommandHandler.cpp`, `Channel.cpp`, `Channel.hpp`.
* **Por qué:** Es la parte más compleja de la lógica de canales y es un requisito mandatorio.

### 3.3. Gestión de Desconexiones (`QUIT` y `PART`)
* **Qué hacer:**
    * Implementar el comando `PART` para que un usuario pueda salir de un canal.
    * Implementar el comando `QUIT` para desconectar a un usuario del servidor, asegurándose de que sale de todos los canales en los que estaba y se notifica a los demás.
    * Mejorar `_removeClient` para que, antes de cerrar el socket, se gestione una desconexión "limpia".
* **Ficheros a modificar:** `CommandHandler.cpp`, `Server.cpp`, `Channel.cpp`.
* **Por qué:** Asegura que el estado del servidor se mantiene consistente cuando los usuarios se van.

### 3.4. Pruebas de Estrés y Errores
* **Qué hacer:** Probar exhaustivamente todos los casos límite:
    * Enviar comandos con parámetros incorrectos o insuficientes.
    * Un usuario no registrado intentando unirse a un canal.
    * Un usuario normal intentando usar comandos de operador.
    * Comprobar todas las respuestas de error numéricas.
* **Ficheros a modificar:** El código en general, añadiendo comprobaciones y respuestas de error.
* **Por qué:** El `subject` es muy claro en que el programa no debe crashear bajo ninguna circunstancia.

---

## Fase 4 (Opcional): Extras (Bonus)

**Objetivo:** Añadir funcionalidades extra si la parte obligatoria es perfecta.

### 4.1. Implementar un Bot
* **Qué hacer:** Crear un "pseudo-cliente" dentro del servidor que pueda responder a comandos específicos (ej: `!help`, `!time`).
* **Ficheros a modificar:** `Bot.cpp`, `Bot.hpp`, `CommandHandler.cpp`.

### 4.2. Transferencia de Ficheros (DCC)
* **Qué hacer:** Implementar el protocolo DCC para permitir la transferencia de archivos entre clientes, gestionada a través del servidor.
