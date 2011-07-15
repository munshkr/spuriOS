% Kernel SMP - PSO
% Liptak, Leandro - leandroliptak@gmail.com

Introducción
============

El objetivo del presente trabajo era extender el _kernel_ desarrollado durante la cursada de la materia _Programación de Sistemas Operativos_ (PSO) dotándolo de la capacidad para operar y sacar provecho en un entorno de multiprocesamiento simétrico (_Symmetric Multiprocessing_, SMP), más precisamente en un entorno de los provistos por procesadores _Intel_ o compatibles.

Una de las principales características de la configuración y puesta en marcha de los diversos procesadores en estos esquemas antes mencionados es que resulta dependiente de las diversas arquitecturas. Es decir, cada arquitectura puede requerir implementaciones distintas para la inicialización de los procesadores o la comunicación entre los mismos. Las arquitecturas que requieren distinción son: _Intel 486DX2_, _Intel P6_ e _Intel P4_. Dichas arquitecturas están ligadas a diferentes modelos de APIC (_Advanced Programable Interrupt Controller_), de aquí las consideraciones.

Lineamientos generales
======================

Para convertir nuestro _kernel_ en SMP (abusando de aquí en más de dicha expresión) lo primero y más fundamental es reunir la información necesaria acerca de los diversos procesadores en el sistema. Una vez que dichos procesadores se han identificado y se han creado las estructuras de datos pertinentes, es necesario inicializarlos para que se vuelvan operables, pues por defecto dichos procesadores (exceptuando el procesador de arranque, o _Bootstrap Processor_, BSP) se encuentran suspendidos. Finalmente, es necesario modificar los mecanismos de planificación de procesos (es decir, el _scheduler_ del sistema) para que saque provecho de los diversos procesadores y los administre eficientemente como recurso. Sin embargo en el transcurso de lograr muchas de estas metas problemas inherentes surgirán como ser, por ejemplo, la sincronización en estos esquemas multiprocesador.

Implementación
==============

Identificación del sistema
--------------------------

Parseo de la tabla MP basica y extendida, etc. Codigo del kernel de Linux. Estructuras de datos creadas.

Inicialización de los procesadores
----------------------------------

Sequencia de arranque, INIT IPI, SIPI segun modelo 486DX2 o P4, APIC o xAPIC. _Warm reset_, _code vector_, _stacks_.

APIC
----

Identificación, esquema de uso y facilidades implementadas.

Spinlocks
---------

Spinlocks, ventajas y porqué son necesarios.

Scheduling
----------

Multiples colas, organizacion.

Balanceo de carga
-----------------

Balanceo de carga en `fork()`, `run()` (?)

Conclusiones
============

Trabajo futuro
==============
