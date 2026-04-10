# ft_irc - Plan Final

El núcleo del servidor, la refactorización arquitectónica y la Tarea A (Gestión de Canales) están completamente terminados e integrados en la rama principal. 

Este documento organiza el trabajo restante del proyecto, enfocado exclusivamente en la estabilidad extrema y los requisitos del Bonus (Tarea B).

## Estado Actual (Completado)

La arquitectura ya no depende de un `Server` monolítico. La red está aislada en `Socket` y `EventManager`, y la lógica en `CommandHandler`.

* **Comandos RFC 2812 Obligatorios:** `PASS`, `NICK`, `USER`, `JOIN`, `PART`, `PRIVMSG`, `QUIT`, `KICK`, `INVITE`, `TOPIC` y `MODE` (con flags `i`, `t`, `k`, `o`, `l`) totalmente funcionales.
* **Soporte de Listas:** `JOIN` y `KICK` soportan múltiples canales y usuarios separados por comas.
* **Utilidades de Compatibilidad Implementadas:**
  * `CAP LS`: Evita que clientes modernos (HexChat, irssi) se queden congelados en el handshake inicial.
  * `PING` / `PONG`: El servidor responde a los pings de actividad, evitando desconexiones por timeout en clientes oficiales.
  * `NOTICE`: Implementado para el envío de mensajes sin respuesta automática (vital para el Bot).

---

## Tarea B: Robustez y Bonus

El objetivo de esta fase es someter el servidor a pruebas de estrés para garantizar que no haya *segfaults* ni *leaks* bajo ninguna circunstancia, y desarrollar las dos características del Bonus.

### 1. Auditoría y Pruebas de Estrés
Utilizar valgrind para detectar fugas de memoria y segfaults en una terminal:
``` bash
valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes --track-origins=yes ./ircserv 6667 <password>
```
Mientras que en otra(s) utilizamos netcat para intentar romper el servidor:
 ``` bash
 nc -C 127.0.0.1 6667
 ```

Ejemplos:
* **Entradas malformadas:** Comandos sin argumentos, parámetros insuficientes, nicks con caracteres inválidos.
* **Fragmentación TCP:** Enviar comandos partidos en varios paquetes (usando Ctrl+D en netcat) para verificar que el buffer de reconstrucción funciona hasta encontrar el `\r\n`.
* **Límites estrictos:** Verificar el comportamiento al superar el límite de caracteres por comando (512 bytes) y probar desconexiones abruptas masivas (10+ clientes cerrando a la vez) validando con `valgrind`.

### 2. Desarrollo del Bot (Bonus)
Integrar un "pseudo-cliente" en el servidor que monitorice los canales y responda a comandos.
* **Comandos mínimos:** `!help`, `!time`, `!ping`.
* **Regla de Arquitectura:** El bot debe emitir sus respuestas utilizando el comando `NOTICE` (ya implementado), nunca `PRIVMSG`. Esto evita bucles infinitos si el bot interactúa accidentalmente con otro bot.

### 3. Transferencia de Archivos DCC (Bonus)
Implementar el protocolo DCC SEND para permitir el envío de archivos entre dos clientes.
* El servidor debe ser capaz de procesar e intermediar el mensaje CTCP inicial (`PRIVMSG target :\x01DCC SEND ...\x01`) para que los clientes establezcan la conexión punto a punto.

---

## Compilación y Ejecución

```bash
# Compilar el binario
make

# Iniciar el servidor
./ircserv <port> <password>

# Ejemplo: ./ircserv 6667 pass123
```

---

## Uso con netcat (nc)

Para probar el servidor de forma manual sin un cliente gráfico, utiliza netcat. Es importante usar el flag -C para enviar los terminadores de línea \r\n que exige el protocolo.
``` bash

# Conexión inicial
nc -C 127.0.0.1 <port>

# Comprobar que el servidor está escuchando (no necesario)
PING :<message>

# Secuencia de registro (necesario para registrarse en el servidor)
PASS <server_password>
NICK <nickname>
USER <username> 0 * :<realname>

# Tras recibir el mensaje de bienvenida (001), puedes usar el resto de comandos:
JOIN <channel>
PRIVMSG <target> :<message>
WHOIS <nickname>
QUIT :<reason>
```