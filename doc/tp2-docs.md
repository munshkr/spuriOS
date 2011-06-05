% Trabajo Práctico 2 ~ PSO
% Liptak, Leandro - leandroliptak@gmail.com; Reboratti, Patricio - darthpolly@gmail.com; Silvani, Damián - dsilvani@gmail.com

Documentación
=============

Char Devices
------------

### `device` - Devices

La mayor parte del diseño del módulo `device` se centró en cómo almacenar los descriptores de archivos y en como debían ser implementadas las llamadas al systema. Respecto al primer punto, decidimos almacenar los FDs (file descriptors) en una matriz de PID por FD, acotada por el máximo process id y máximo file descriptor (ambos con valor actual 32). Dicha matriz contiene punteros a descriptores de dispositivos y se considera una entrada disponible aquella con valor 0 (puntero nulo). Entonces, la cantidad de memoria empleada por dicha estructura es 32 * 32 * 4 bytes = 4KB. Fue fundamental que recordásemos la necesidad de cerrar los descriptores de archivo que el usuario pudiese dejar abiertos al llamar a `loader_exit` pues de lo contrario, ciertos dispositivos como el puerto serie (de apertura exclusiva) jamás hubiesen podido ser reabiertos por ningún otro proceso.

Es importante mencionar la implementación de una función `copy2user` (acompañada de otras, por ejemplo, para brindar el nivel de privilegio de cierta dirección virtual) utilizada en todos los char devices. Aún así, sabemos que dicha función no es perfecta y no está libre de bugs de seguridad, pero es un primer intento (aprovechamos por ejemplo el hecho de que el kernel se encuentra en direcciones inferiores a cualquiera de usuario para evitar validar todas las direcciones en cierto rango).

### `con` - Driver de consola

El driver de consola administra un anillo de multiples **consolas**. Cada una de
estas representa un *char device* con algunos atributos extras que permiten
almacenar el estado de la consola en un momento dado, una cola para la lectura
del teclado y punteros a otros dispositivos de consola, definiendo el orden de
las consolas en el anillo. Se pueden tener abiertas 8 consolas como máximo (los
*devices* se almacenan en un arreglo fijo en el módulo).

El **estado** de una consola comprende un buffer de 4000 bytes^[80 filas x 25
columnas x 2 bytes por caracter = 4000 bytes] y una estructura
`vga_screen_state_t` que almacena la posición del cursor (fila y columna) y los
atributos de color. Además del estado, cada consola tiene puntero a la
*siguiente* y *anterior* consola en el anillo.

En la inicialización, el driver inicializa el arreglo de consolas y registra un
*handler* de interrupción del teclado. Este handler lee 1 byte (el scancode) del
puerto de control del teclado, y actualiza unas variables globales que contienen
el último scancode leído, el último caracter leído y si las teclas de control
`ALT`, `SHIFT` o `CTRL` están siendo presionadas. Una vez que esta información
esta disponible, chequea si la combinación `ALT`+`SHIFT`+`Izquierda` o
`ALT`+`SHIFT`+`Derecha` se presionó, y hace un *cambio de consola* a la anterior
o a la siguiente, según corresponda. Si la consola que tiene foco está esperando
al teclado, se la despierta para que pueda procesar el nuevo scancode o
caracter.

En un principio habíamos definido una única cola de lectura de teclado global,
para todas las consolas. Pero esto no iba a ser posible por que se podía dar la
situación en la que varias tareas tienen una consola diferente abierta, leyendo
del teclado, y sólo habría que despertar a la tarea que está esperando sobre la
consola que tiene foco, en cuanto la interrupción llegue.

Sólo una cola de lectura es suficiente para una consola, pues, si bien una
tarea puede tener varios file descriptors de consolas distintas, sólo puede
bloquearse a leer de una sola de ellas al mismo tiempo.

Como las tareas no tienen un mecanismo para compartir file descriptors, sólo
una tarea puede estar bloqueada en la cola de una consola a la vez, así que el
estado de esta consola siempre se alternaría entre `FREE_QUEUE` y el pid de una
tarea existente.

Cuando una tarea abre una nueva consola, se crea e inicializa un nuevo
dispositivo, se la agrega al anillo justo después de la consola que tiene foco
en ese momento, y finalmente se cambia el foco a la nueva. Cuando se cierra la
consola, luego de eliminar el buffer dinamico y corregir los punteros de la
lista enlazada, se le da foco a la anterior consola.

En el momento del cambio de foco, se copia el contenido del buffer VGA en
0xB8000 al buffer de la consola, guardando el estado, y se restaura el
contenido del buffer de la nueva consola al buffer VGA. Cuando se escribe al
*file descriptor* de una consola, si ésta tiene foco, se escribe directamente
al buffer VGA en 0xB8000, y sino al buffer de la consola. De esta manera, sólo
escribimos una sola vez, sea en el buffer de video real, o el de la consola, y
no ambos al mismo tiempo.

### `serial` - Driver del puerto serie

El controlador del puerto serie es extremadamente sencillo y de hecho no hace uso de todas las funcionalidades que el dispositivo ofrece. Las lecturas de este dispositivo son de a byte, es decir, no importa el tamaño especificado a leer, en caso de éxito se devolverá un byte al usuario por cada llamada `read`. Esta decisión libera al driver de la responsabilidad de tener un buffer interno y un puntero al mismo.

El descriptor de dispositivo cuenta con una cola de espera de lectura y otra de escritura. El estado de estas colas es siempre vacío o bien con un proceso encolado. Esto se debe a que tomamos la decisión de que la apertura del puerto serie fuese exclusiva: dos procesos no pueden tener al mismo tiempo un descriptor a éste. Dicho esto, nos queda la duda si quizás una cola hubiese bastado, pues al ser bloqueantes las lecturas y escrituras, una para cada operación se torna innecesario.

Respecto a las funcionalidades del dispositivo, no utilizamos el sistema de colas presente en el mismo, lo cual simplifica el manejo de interrupciones por parte del driver. A su vez, configuramos los parametros de conexión (baudios, paridad, tamaño del caracter, etc.) al momento de la inicialización de los dispositivos y permanecen configurados de esta forma a lo largo de la ejecución. Estos parametros son: 8 bits de tamaño del caracter, sin paridad, un bit de parada y 9600 baudios. Cabe mencionar que la dirección del puerto de E/S es almacenada en el descriptor de dispositivo en un intento de proveer mayor flexibilidad al controlador.

Durante el desarrollo afrontamos un problema que permanece sin solución: no logramos recibir una interrupción desde los puertos 3 y 4. Consultando con los docentes concluimos que es probable que bochs no tuviese implementada dicha funcionalidad.

Block Devices
-------------

### `hdd` - Hard Disk Driver

Nos limitamos a implementar sólo la la funcionalidad de lectura del disco rígido, para el canal primario del mismo (primary master, primary slave). Cualquier lectura fuera del tamaño de bloque del dispositivo da como resultado un error.

El controlador de dispositivo efectúa PIO (entrada/salida programada) utilizando LBA de 28 bits y con lectura sencilla (no múltiple, es decir, un comando de lectura efectúa la lectura de un sólo sector de disco). Durante el desarrollo nos encontramos con ciertas dudas acerca de los retardos necesarios antes del envío de cada comando al dispositivo (teóricamente de 400ms), pues si bien en bochs el controlador funciona, parece no funcionar en otros entornos como VirtualBox con un chipset y controladores idénticos.

El descriptor de dispositivo alberga el puerto de E/S primario y el puerto auxiliar de control, así como el canal (maestro, esclavo) y una cola de espera por lecturas. En dicha cola podría haber cero o más procesos. Cabe destacar que no es necesario un puntero para la cantidad de bytes copiados así como tampoco se utiliza la función `copy2user` pues se asume que dicho controlador es utilizado por el kernel y no directamente por el usuario.

File System
-----------

### `fs` - Sistema de archivos

### Implementación del sistema de archivos

Tareas
------

### `run` syscall

Durante la implementación de dicha llamada al sistema nos topamos con una limitación: `loader_load` esperaba el puntero a un archivo PSO que eventualmente podría tener un tamaño mayor al de una página. Esto se traduce en la necesidad de direcciones virtuales contiguas y debimos implementar funciones en el módulo de manejo de memoria para tal fin. Lo que finalmente hicimos fue buscar una entrada vacía en el directorio de páginas del proceso actual, lo cual nos brindaba un espacio de direccionamiento virtual contiguo de hasta 4MB. De aquí se deduce la limitación actual para utilizar dicha llamada al sistema: el ejecutable más grande no puede exceder los 4MB. Una vez ejecutada la carga del archivo se liberan los recursos previamente reservados.

### tarea `init`

### tarea `console`

