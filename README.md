# ft_irc: Implementación de un Servidor IRC en C++98

Este repositorio contiene el código fuente de `ircserv`, un servidor de Internet Relay Chat (IRC) desarrollado desde cero en C++98. El proyecto se adhiere a las especificaciones fundamentales del protocolo IRC, permitiendo a múltiples clientes conectarse, unirse a canales y comunicarse en tiempo real.

La principal restricción técnica y, a su vez, el núcleo del diseño, es el uso de **entrada/salida no bloqueante** y la multiplexación de eventos a través de `poll()`. Esto permite al servidor gestionar un gran número de conexiones concurrentes en un único hilo de ejecución, sin recurrir a `fork()` ni a `threads`, tal y como exige el `subject` del proyecto.

---

## 🏗️ 1. Arquitectura y Filosofía de Diseño

La arquitectura del servidor se fundamenta en el **Principio de Responsabilidad Única (SRP)**. Cada clase tiene un propósito claro y bien definido, lo que resulta en un sistema más modular, mantenible y escalable.

### `Server`
Es la clase orquestadora. Actúa como el núcleo central que gestiona toda la infraestructura de red y el ciclo de vida de los clientes.

* **Responsabilidades Clave:**
    * **Inicialización:** Configura y enlaza el socket de escucha principal.
    * **Bucle de Eventos:** Ejecuta el bucle infinito que espera y despacha eventos de red usando `poll()`.
    * **Gestión de Clientes:** Acepta nuevas conexiones, crea los objetos `Client` correspondientes y los destruye cuando se desconectan.
    * **Punto de Acceso:** Proporciona a otras partes del sistema (como el `CommandHandler`) los mecanismos para interactuar con los clientes (ej: enviar mensajes).

* **Atributos Notables:**
    * `_server_fd`: El file descriptor del socket de escucha. Es el único socket que acepta nuevas conexiones.
    * `std::vector<struct pollfd> _fds`: El corazón de la concurrencia. Este vector contiene no solo el `_server_fd`, sino los `fd` de todos los clientes conectados. `poll()` lo utiliza para monitorizar qué socket tiene datos listos para ser leídos.
    * `std::map<int, Client *> _clients`: Un mapa que asocia el `file descriptor` de un cliente con su objeto `Client`. Este diseño es crucial para poder acceder de forma eficiente al estado de un cliente (`O(log N)`) a partir de su `fd`.

### `Client`
Representa a un usuario conectado. Cada instancia encapsula el estado completo y la información asociada a una única conexión.

* **Responsabilidades Clave:**
    * **Gestión de Estado:** Almacena información vital como el nickname, username, y si el cliente ha completado el proceso de autenticación.
    * **Gestión de Búfer:** Mantiene un búfer de entrada (`_buffer`) para ensamblar comandos completos. Esto es **fundamental** para un servidor TCP, ya que los datos pueden llegar fragmentados. La clase se encarga de acumular datos hasta que se recibe un delimitador `\r\n`.

* **Atributos Notables:**
    * `int _fd`: El file descriptor del socket del cliente, que lo identifica unívocamente a nivel de red.
    * `std::string _buffer`: Búfer de acumulación para los datos recibidos vía `recv()`.
    * `_nickname`, `_username`, `_is_authenticated`: Atributos que definen la identidad y el estado del usuario dentro del protocolo IRC.

### `CommandHandler`
Es el cerebro lógico del protocolo IRC. Su única misión es desacoplar la lógica de red de la lógica de la aplicación.

* **Responsabilidades Clave:**
    * **Parseo:** Recibe una cadena de comando completa (ej: `PRIVMSG #canal :Hola!`) y la descompone en sus componentes: prefijo, comando y parámetros.
    * **Despacho (Dispatching):** Utiliza un sistema (típicamente un `std::map`) para asociar el nombre de un comando con la función miembro que debe ejecutarlo. Esto evita un bloque `if-else` masivo y hace que añadir nuevos comandos sea trivial.
    * **Ejecución:** Invoca la lógica específica para cada comando, interactuando con las clases `Client` y `Channel` para modificar el estado del servidor.

### `Channel`
Representa un canal de chat (`#nombre_canal`).

* **Responsabilidades Clave:**
    * **Gestión de Miembros:** Mantiene una lista de los clientes que se han unido al canal.
    * **Gestión de Estado:** Almacena propiedades del canal como el topic, la clave (contraseña), y los modos activos (`+i`, `+t`, `+k`, `+l`).
    * **Gestión de Operadores:** Distingue entre usuarios normales y operadores del canal, aplicando los permisos correspondientes.
    * **Difusión de Mensajes:** Se encarga de reenviar un mensaje recibido de un miembro a todos los demás miembros del canal.

---

## 🔄 2. Ciclo de Vida del Servidor y Flujo de Datos

El funcionamiento del servidor puede entenderse como un ciclo continuo de eventos.

1.  **Arranque (`main` -> `Server::start`)**
    * El `main` valida los argumentos y crea una instancia de `Server`.
    * `Server::start()` invoca a `_setupServerSocket()`, que realiza las llamadas críticas al sistema: `socket()`, `setsockopt(SO_REUSEADDR)`, `fcntl(O_NONBLOCK)`, `bind()` y `listen()`.
    * El `_server_fd` se añade como el primer elemento del vector `_fds`.

2.  **El Bucle de Eventos (`_runEventLoop`)**
    * El servidor entra en un `while(true)`. La llamada a `poll(this->_fds.data(), ...)` bloquea la ejecución del programa de forma eficiente hasta que haya actividad en alguno de los sockets que está vigilando.
    * Cuando `poll()` retorna, el programa itera sobre el vector `_fds` para ver qué descriptores de fichero han generado eventos.

3.  **Flujo de una Nueva Conexión**
    * Si `_fds[0].revents & POLLIN` es verdadero, significa que un nuevo cliente está intentando conectarse.
    * Se llama a `_handleNewConnection()`.
    * `accept()` crea un nuevo socket (`client_fd`) para la comunicación con este cliente.
    * Se crea una nueva instancia `new Client(client_fd)`.
    * El nuevo `client_fd` y el puntero al `Client` se registran en `_fds` y `_clients` respectivamente. El cliente ahora forma parte del bucle de eventos.

4.  **Flujo de Datos de un Cliente Existente**
    * Si `_fds[i].revents & POLLIN` (con `i > 0`) es verdadero, un cliente ya conectado ha enviado datos.
    * Se llama a `_handleClientData(i)`.
    * `recv()` lee los datos del socket.
    * **(Lógica Futura)** Los datos leídos se añaden al `_buffer` del objeto `Client` correspondiente. Un bucle interno comprobará si el búfer contiene uno o más comandos completos (delimitados por `\r\n`).
    * Por cada comando completo extraído, se llamará a `commandHandler.handleCommand(client, command_string)`.

5.  **Flujo de una Desconexión**
    * Si `recv()` en `_handleClientData` retorna `0` (desconexión limpia) o `-1` (error), se inicia el proceso de limpieza.
    * Se llama a `_removeClient(i)`.
    * `close(client_fd)` libera el recurso a nivel de sistema operativo.
    * `delete this->_clients[client_fd]` libera la memoria del objeto `Client`.
    * Finalmente, las entradas correspondientes en el mapa `_clients` y en el vector `_fds` son eliminadas para mantener el estado del servidor consistente.

---

## 🛠️ 3. Cómo Compilar y Ejecutar

El proyecto incluye un `Makefile` para facilitar la compilación.

1.  **Compilar el proyecto:**
    ```bash
    make
    ```
    Esto generará un ejecutable llamado `ircserv` en la raíz del proyecto.

2.  **Ejecutar el servidor:**
    ```bash
    ./ircserv <port> <password>
    ```
    * `<port>`: El puerto en el que el servidor escuchará (ej: 6667).
    * `<password>`: La contraseña que los clientes necesitarán para conectarse.

3.  **Limpiar los ficheros objeto y el ejecutable:**
    ```bash
    make clean  # Elimina los ficheros objeto (*.o)
    make fclean # Llama a clean y además elimina el ejecutable
    make re     # Llama a fclean y vuelve a compilar
    ```

---

## 📊 4. Estado Actual y Próximos Pasos

El proyecto se encuentra en una fase donde la **infraestructura de red es funcional y robusta**.

* **✅ Completado:**
    * Estructura general del proyecto (`Makefile`, directorios, etc.).
    * Arquitectura de clases definida (`Server`, `Client`, `CommandHandler`, `Channel`).
    * Lógica completa para aceptar, gestionar y desconectar múltiples clientes de forma asíncrona usando `poll()`.

* **🚧 Próximos Pasos (Fase 1 del `roadmap.md`):**
    1.  **Implementar la gestión de búfer:** Modificar `_handleClientData` para que acumule datos en `Client::_buffer` y extraiga comandos completos.
    2.  **Desarrollar el `CommandParser`:** Crear la lógica que descompone una cadena de comando en sus partes.
    3.  **Implementar el `CommandDispatcher`:** Construir el mapa en `CommandHandler` que asocia strings de comando a funciones.
    4.  **Crear API de respuesta:** Desarrollar una función centralizada (`Server::sendReply`) para enviar mensajes a los clientes.

Una vez completada esta fase, la base estará lista para que comience la implementación de la lógica específica de cada comando IRC (Fases 2 y 3 del `roadmap`).
