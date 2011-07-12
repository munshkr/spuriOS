% Trabajo Práctico 3 ~ PSO
% Liptak, Leandro - leandroliptak@gmail.com; Reboratti, Patricio - darthpolly@gmail.com; Silvani, Damián - dsilvani@gmail.com

Documentación
=============

Inter-process Communication
---------------------------

### `pipe` - Pipes

Un pipe está compuesto por dos char devices denominados _endpoints_ o extremos.
Cada extremo tiene un puntero a un buffer común de tamaño fijo (4Kb), es decir,
el buffer del pipe, un puntero a su extremo opuesto, y una cola. Uno de los
extremos sólo puede leer del buffer, mientras que el otro sólo puede escribir.
Los char devices se almacenan en un arreglo de tamaño fijo.

El buffer está implementado como un arreglo de 4Kb circular. Para mayor
eficiencia en la utilización del espacio, el buffer se utiliza _circularmente_:
Cada char device extremo tiene un puntero de posición y un entero que indica la
cantidad de bytes utilizados o disponibles para leer. Los punteros son
individuales a cada extremo, pero el entero de "bytes disponibles" es
compartido. Cuando el extremo de escritura almacena *n* bytes, se incrementa la
variable de "bytes disponibles" y se avanza el puntero. Cuando un puntero se
excede del límite del buffer, comienza de nuevo al principio del arreglo (_wrap
around_). Por otro lado, el extremo de lectura sólo puede leer hasta que su
puntero de posición no sobrepase el de escritura (puesto que ya no hay más
bytes para leer, el resto es basura).

Cuando se intenta leer del buffer y éste está vacío, el proceso se bloquea
hasta que el buffer contenga nueva información, y esto únicamente sucede cuando
otro proceso escribe en el pipe. Al terminar de escribir, el proceso despierta
a todos los que están esperando en la cola del otro extremo, el de lectura. El
proceso es análogo cuando un proceso intenta escribir en un buffer lleno. De
esta manera, el mecanismo de pipes se utiliza de manera transparente para los
procesos, éstos no necesitan conocer el tamaño del buffer interno. Más
importante aún, permite una manera de sincronizar la comunicación entre dos o
más procesos de manera óptima, puesto que los procesos se van a dormir cuando
tienen que esperar para procesar la información proveniente de otros procesos,
y se despiertan exactamente cuando la información está disponible.

La función de escritura devuelve 0 inmediatamente cuando el otro extremo par se
cierra. Este comportamiento es conocido como _broken pipe_. La razón es simple:
no tiene sentido escribir en un buffer que ya no se puede leer. Por otro lado,
en la función de lectura se suma la condición de que el buffer esté vacío. Es
decir, si un proceso escribe en el pipe y cierra el extremo de escritura, el
proceso que lee del pipe puede leer lo que queda del buffer antes de que la
comuncación se corte.

### `loader` - Extensión a la carga de procesos

Implementar la llamada al sistema `fork()` resultó más sencillo que la
implementación de `loader_load` o `run` en parte por la capacidad de reutilizar
funcionalidades diseñadas para estas últimas. Se solicita un nuevo PID, un nuevo
directorio de páginas (con el área del kernel mapeada) y se procede a crear el
PCB para el proceso hijo. Luego (según la primer implementación de la syscall)
se copian todas aquellas páginas presentes en el esquema de paginación del
proceso padre y se mapean dichos frames con el contenido copiado y la misma
dirección virtual en el esquema de paginación del hijo. La excepción a dicha
copia son las páginas de kernel del área de 0 a 4MB y la página de la pila de
anillo cero pues el contenido de ésta no era el adecuado para un proceso recién
lanzado.

Posteriormente, se arma una pila de anillo 0 tal como lo haría `loader_load` y
se mapea en la dirección esperada. Finalmente, se copian los descriptores de
archivos incrementando el número de referencias en el descriptor de dispositivo
asociado y se encola el proceso en el scheduler.

Una vez implementadas las funcionalidades vinculadas al manejador de memoria,
fue necesario pulir el mecanismo de copia de páginas del proceso padre al hijo
pues en las entradas de las tablas de páginas había mucha más información que la
simple presencia de la página. Por este motivo la copia se extendió también a
las entradas de las tablas de páginas transportando así la información presente
en ellas al nuevo proceso. Vale aclarar que en algunos casos no se trata de una
copia directa de la entrada sino que la información presente en ella/s puede
verse alterada.

Manejador de memoria
--------------------

### Memoria on-demand

Brindar dicha funcionalidad fue relativamente sencillo: a medida que se
solicitaban páginas con `palloc()` el rango de direcciones virtuales crecía pero
la solicitud del frame correspondiente se postergaba. Es así que la entrada en
la tabla de páginas correspondiente a dicha página continuaba marcada como no
presente pero uno de los bits restantes de dicha entrada se seteaba indicando
que la página había sido previamente solicitada. Esto permitía posteriormente,
frente al fallo producido a causa del acceso a dicha página poder discernir
entre un intento de acceso a una dirección inválida o a una página previamente
demandada. Esta lógica se incluyó en el handler del _Page Fault_ y si el caso
fuera el último mencionado se procede a solicitar un frame y mapearlo en dicha
dirección virtual, retornando luego a la instrucción que causó la excepción.

### Memoria compartida

### Copy-on-write

Para implementar el mecanismo de _copy-on-write_ se modificó, como se ha
mencionado, aquella sección de la llamada al sistema `fork()` en que se copiaban
las páginas de un proceso a otro. En lugar de esto, se procedió a crear el mapeo
en el esquema de paginación del hijo, para la misma dirección virtual que el
padre e indicando la página como presente, pero con acceso de sólo lectura.
Tambíen la entrada del padre es marcada como de sólo lectura y adicionalmente
ambas entradas se marcan indicando que han de ser copiadas ante una escritura.

Posteriormente, frente a una eventual escritura se corrobora si la página debe
ser copiada o no. En caso de haber sido marcada para ser copiada, pero ser el
único proceso con dicho mapeo (pues los otros ya han accedido y han hecho la
copia), dicha página se otorga nuevamente con permisos de lectura/escritura sin
ser copiada. En otro caso, la copia se efectúa, indicando luego que dicha página
ya no necesita copia en el esquema de paginación de dicho proceso.

Tareas
------
