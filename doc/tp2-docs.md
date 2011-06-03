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
`ALT`, `SHIFT` o `CTRL` están siendo presionadas. Una vez que se tiene esta
información, chequea si la combinación `ALT`+`SHIFT`+`Izquierda` o
`ALT`+`SHIFT`+`Derecha` se presionó, y hace un *cambio de consola* a la anterior
o a la siguiente, según corresponda. Si la consola que tiene foco está esperando
al teclado, se la despierta para que pueda procesar el nuevo scancode o
caracter.

### `serial` - Driver del puerto serie

Block Devices
-------------

### `fdd` - Floppy Disk Driver

### `hdd` - Hard Disk Driver

File System
-----------

### `fs` - Sistema de archivos

### Implementación del sistema de archivos

Tareas
------

