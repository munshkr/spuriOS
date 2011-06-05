% Trabajo Práctico 2 ~ PSO
% Liptak, Leandro - leandroliptak@gmail.com; Reboratti, Patricio - darthpolly@gmail.com; Silvani, Damián - dsilvani@gmail.com

Documentación
=============

Char Devices
------------

### `device` - Devices

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

Block Devices
-------------

### `hdd` - Hard Disk Driver

File System
-----------

### `fs` - Sistema de archivos

### Implementación del sistema de archivos

Tareas
------

