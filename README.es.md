_This project has been created as part of the 42 curriculum by smarin-a, jdelorme y integrnate aun por confirmar_

# ft_irc

## Descripción
`ft_irc` es un servidor de Internet Relay Chat (IRC) implementado en C++98. Este proyecto tiene como objetivo replicar la funcionalidad de un servidor IRC real, permitiendo a los usuarios conectarse, unirse a canales y enviar mensajes en tiempo real, todo ello gestionando sockets no bloqueantes y I/O multiplexing.

## Instrucciones

### Compilación
Para compilar el proyecto, ejecuta el siguiente comando en la raíz del repositorio:

```bash
make
```

Esto generará el ejecutable `ft_irc`.

### Ejecución
Para iniciar el servidor, utiliza el siguiente formato:

```bash
./ft_irc <port> <password>
```

*   `<port>`: El puerto en el que el servidor escuchará conexiones entrantes (e.g., 6667).
*   `<password>`: La contraseña requerida para que los clientes se conecten.

Ejemplo:
```bash
./ft_irc 6667 mi_contraseña_segura
```


## Funciones Clave del Sistema (ft_irc)

Este proyecto es una implementación de un servidor IRC escrito en **C++98**, utilizando APIs de redes de UNIX de bajo nivel.
A continuación se presenta un breve resumen de las funciones del sistema más importantes utilizadas en el proyecto y su papel dentro del servidor.

### Creación y configuración de sockets

- **socket()**
  Crea un socket TCP que será utilizado por el servidor para aceptar conexiones entrantes.

- **setsockopt()**
  Se utiliza para configurar opciones de socket como `SO_REUSEADDR`, permitiendo reiniciar el servidor sin esperar a que el puerto sea liberado.

- **bind()**
  Asocia el socket con una dirección IP y puerto específicos proporcionados como argumentos del programa.

- **listen()**
  Marca el socket como pasivo, habilitándolo para aceptar conexiones entrantes de clientes.

### Manejo de conexiones de clientes

- **accept()**
  Acepta una nueva conexión entrante y devuelve un nuevo descriptor de archivo dedicado a un solo cliente. Cada cliente conectado tiene su propio socket.

- **close()**
  Cierra un socket de cliente o servidor y libera los recursos asociados.

### Transmisión de datos

- **recv()**
  Recibe bytes crudos desde un socket de cliente. Dado que TCP se basa en flujos, los datos pueden llegar parcialmente o combinados, requiriendo buffering y reconstrucción de mensajes.

- **send()**
  Envía datos a un socket de cliente. El envío puede ser parcial, por lo que los mensajes salientes se almacenan en buffers de salida hasta que se transmiten completamente.

### Multiplexación y E/S no bloqueante

- **poll()** (o equivalente)
  El núcleo del bucle del servidor. Permite monitorear múltiples sockets simultáneamente para eventos de lectura/escritura usando un único mecanismo de sondeo, sin bloquear el proceso.

- **fcntl()**
  Utilizado para establecer sockets en modo no bloqueante (`O_NONBLOCK`), asegurando que el servidor nunca se bloquee en operaciones de E/S.
  En macOS, esto es requerido para lograr un comportamiento no bloqueante consistente.

### Utilidades de orden de bytes de red

- **htons() / htonl()**
  Convierten valores del orden de bytes del host al orden de bytes de la red (usado para puertos e IPs).

- **ntohs() / ntohl()**
  Convierten valores del orden de bytes de la red de vuelta al orden de bytes del host.

### Utilidades de dirección y protocolo

- **getaddrinfo() / freeaddrinfo()**
  Resuelven nombres de host y servicios en estructuras de dirección de socket utilizables (IPv4 / IPv6).

- **inet_addr(), inet_ntoa(), inet_ntop()**
  Convierten direcciones IP entre formatos binarios y legibles por humanos, utilizados principalmente para depuración y registro.

### Manejo de señales (opcional)

- **signal(), sigaction()**
  Utilizados para manejar señales del sistema como `SIGINT`, permitiendo que el servidor se apague limpiamente cuando es interrumpido.

### Notas

- Todas las operaciones de E/S son no bloqueantes.
- Solo se utiliza una llamada a `poll()` para gestionar todos los sockets.
- El servidor está diseñado como un bucle impulsado por eventos sin `fork()` ni hilos.
- Los datos entrantes se almacenan en buffers por cliente y se reconstruyen en comandos IRC completos antes de su análisis y ejecución.

## Recursos

Este proyecto se ha desarrollado siguiendo los estándares oficiales del protocolo IRC:

*   [RFC 1459](https://tools.ietf.org/html/rfc1459) - Protocolo IRC (Internet Relay Chat)
*   [RFC 2812](https://tools.ietf.org/html/rfc2812) - Arquitectura Cliente-Servidor IRC
