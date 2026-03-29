# ft_irc - Plan de Trabajo Final

Este documento es útil durante el desarrollo del proyecto, no será entregado en la evaluación. El README.md que pide entregar el subject y explica en sí el proyecto completo actualmente se llama [README_entrega.md](./README_entrega.md).

El núcleo del servidor ya es completamente funcional. Toda la infraestructura de red no bloqueante (`poll`), la arquitectura de clases (`Server`, `Client`, `Channel`) y el flujo básico del protocolo IRC están terminados y estables.

Este documento organiza el trabajo restante del proyecto, dividiéndolo en dos bloques de trabajo independientes para poder avanzar en paralelo sin generar conflictos de código.

## Estado Actual (No requiere modificaciones)

* **Red:** Conexiones concurrentes y multiplexación (I/O no bloqueante) sin crasheos ni fugas de memoria.
* **Autenticación:** Secuencia de registro completa (`PASS`, `NICK`, `USER`).
* **Chat Básico:** Envío de mensajes (`PRIVMSG`), gestión básica de canales (`JOIN`, `PART`) y desconexiones seguras (`QUIT`).
* **Gestión de Memoria:** Limpieza automática de canales vacíos y de clientes desconectados.

---

## Reparto de Tareas

Para maximizar la eficiencia y evitar problemas al fusionar el código, el trabajo se divide en dos áreas con enfoques distintos, siendo ideal completar primero la Tarea A:

### Tarea A: Gestión Avanzada de Canales (Operadores y Modos)

**Objetivo:** Implementar los comandos de administración de canales exigidos en los requisitos obligatorios, asegurando coincidencia plena de las respuestas del servidor con el estándar RFC 2812

**Archivos principales:** `CommandHandler.cpp`, `Channel.cpp` y `Replies.hpp`.

* **Comandos de gestión:** Desarrollar la lógica de ejecución para `KICK` (expulsar usuarios del canal), `INVITE` (añadir usuarios a la lista de excepciones para canales privados) y `TOPIC` (consultar o modificar el tema, respetando las restricciones).
* **Comando `MODE`:** Es el núcleo de este bloque. Requiere crear una función que parsee los *flags* enviados por el cliente y los aplique utilizando los métodos ya existentes en la clase `Channel`:
    * `+i` / `-i`: Activar/desactivar canal de solo invitación.
    * `+t` / `-t`: Restringir la modificación del topic solo a operadores.
    * `+k` / `-k`: Establecer o eliminar la contraseña (key) del canal.
    * `+o` / `-o`: Otorgar o retirar privilegios de operador a un miembro.
    * `+l` / `-l`: Establecer o eliminar el límite máximo de usuarios.
* **Respuestas Numéricas (RFC 2812):** Revisar los comandos implementados para garantizar que devuelven el código numérico de error exacto en caso de fallo (ej. falta de privilegios, canal inexistente, sintaxis incorrecta). Habrá que ampliar las macros en `Replies.hpp`.


### Tarea B: Robustez del Protocolo, Errores y Bonus

**Objetivo:** Asegurar la estabilidad ante entradas imprevistas buscando errores y desarrollar las funcionalidades bonus.

**Archivos principales:** Validaciones en `CommandHandler.cpp` y creación de las clases para el Bot y la transferencia de archivos en caso de que sea necesario.

* **Auditoría y Pruebas de Estrés:** Tratar de romper el servidor enviando comandos malformados, cadenas de texto vacías, parámetros insuficientes o provocando desconexiones abruptas. El objetivo es garantizar que no haya *segfaults* o *leaks* bajo ninguna circunstancia.
* **Desarrollo del Bot (Bonus):** Diseñar e integrar un "pseudo-cliente" en el servidor que monitorice los mensajes y responda automáticamente a comandos específicos (por ejemplo, devolviendo información, la hora, o ejecutando un juego simple).
* **Transferencia DCC (Bonus):** Si los tiempos del proyecto lo permiten, investigar la implementación de las transferencias directas de archivos entre clientes.

---

## Compilación y Ejecución

```bash
# Compilar el binario
make

# Iniciar el servidor
./ircserv <puerto> <contraseña>
# Ejemplo: ./ircserv 6667 pass123

```
