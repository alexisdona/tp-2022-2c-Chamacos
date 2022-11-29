# tp-2022-2c-ChamacOS

LINK REPOSITORIO:
[https://github.com/sisoputnfrba/tp-2022-2c-ChamacOS] 

LINK ENUNCIADO:
[https://docs.google.com/document/d/1xYmkJXRRddM51fQZfxr3CEuhNtFCWe5YU7hhvsUnTtg/edit]

LINK PRUEBAS:
[https://docs.google.com/document/d/1qxF4h9dkL5L6X6P8qts997wTC1qLxIg6ElZzlU1Xgi4/edit]
---

## CHECKPOINT #5 ENTREGAS FINALES: MEDRANO

_FECHAS: 26/11/2022 - 03/12/2022 - 17/12/2022_

### Objetivos 📌
```
* Finalizar  el desarrollo de todos los procesos.
* Probar de manera intensiva el TP en un entorno distribuido.
* Todos los componentes del TP ejecutan los requerimientos de forma integral.
```

### Documentacion 📖
```
* Guía de Despliegue de TP - https://docs.utnso.com.ar/guias/herramientas/deploy
* Guía de uso de Bash - https://docs.utnso.com.ar/guias/consola/bash
```

---

## CHECKPOINT #4 AVANCE DEL GRUPO: VIRTUAL

_FECHA: 19/11/2022_

### Objetivos 📌
```
Módulo Kernel (completo):
* Implementa circuito de Page Fault

Módulo CPU (completo):
* Implementa TLB
* Implementa instrucciones MOV_IN, MOV_OUT

Módulo Memoria:
* Implementa tabla de Páginas
* Responde los mensajes de la CPU y Kernel con datos reales
* Respuesta Page Fault de manera genérica sin acciones
```

### Documentacion 📖
```
* Sistemas Operativos, Silberschatz, Galvin 7ma Ed. - Capítulo 8: Memoria principal
* Sistemas Operativos, Stallings, William 5ta Ed. - Parte VII: Memoria virtual (Cap. 8)
* Sistemas Operativos, Silberschatz, Galvin 7ma Ed. - Capítulo 9: Memoria virtual
* Tutorial de Valgrind - https://docs.utnso.com.ar/guias/herramientas/valgrind
```

---

## CHECKPOINT #3 OBLIGATORIO: PRESENCIAL

_FECHA: 05/11/2022_

### Objetivos 📌
```
 Realizar pruebas mínimas en un entorno distribuido.

Módulo Consola (completo):
* Atiende las peticiones del Kernel para imprimir por pantalla e ingresar input por teclado.

Módulo Kernel:
* Planificador de Largo Plazo funcionando, respetando el grado de multiprogramación.
* Planificador de Corto plazo funcionando para los algoritmos RR y Colas Multinivel.
* Manejo de estado de bloqueado sin Page Fault.

Módulo CPU:
* Implementa ciclo de instrucción completo.
* Ejecuta instrucciones SET, ADD, IO y EXIT

Módulo Memoria:
* Responde de manera genérica a los mensajes de Kernel y CPU
```

### Documentacion 📖
```
* Sistemas Operativos, Stallings, William 5ta Ed. - Parte IV: Planificación
* Sistemas Operativos, Silberschatz, Galvin 7ma Ed. - Capítulo 5: Planificación
* Sistemas Operativos, Stallings, William 5ta Ed. - Parte VII: Gestión de la memoria (Cap. 7)
* Guía de Debugging - https://docs.utnso.com.ar/guias/herramientas/debugger
* Guía de Despliegue de TP - https://docs.utnso.com.ar/guias/herramientas/deploy
* Guía de uso de Bash - https://docs.utnso.com.ar/guias/consola/bash
```

---

## CHECKPOINT #2 AVANCE

_FECHA: 01/10/2022_

### Objetivos 📌
```
Módulo Consola:

* Obtener el contenido de los archivos de config y de pseudocódigo.
* Parsear el archivo de pseudocódigo y enviar la información al Kernel.

Módulo Kernel:
* Crea las conexiones con la Memoria, la CPU y atención de las consolas
* Generar estructuras base para administrar los PCB y sus estados con la información recibida por el módulo Consola.
* Planificador de Largo Plazo funcionando, respetando el grado de multiprogramación.
* Planificador de Corto Plazo funcionando con algoritmo FIFO.

Módulo CPU:
* Generar las estructuras de conexión con el proceso Kernel y Memoria.
* Envía  y recibe el contexto de ejecución del módulo Kernel.

Módulo Memoria:
* Generar las estructuras de conexión con los procesos kernel y CPU.
```

### Documentacion 📖
```
* Sistemas Operativos, Stallings, William 5ta Ed. - Parte IV: Planificación
* Sistemas Operativos, Silberschatz, Galvin 7ma Ed. - Capítulo 5: Planificación
* Guía de Buenas Prácticas de C - https://docs.utnso.com.ar/guias/programacion/buenas-practicas
* Guía de Serialización - https://docs.utnso.com.ar/guias/linux/serializacion
* Charla de Threads y Sincronización - https://docs.utnso.com.ar/guias/linux/threads
```

---

## CHECKPOINT #1 INICIAL

_FECHA: 17/09/2022_

### Objetivos 📌
```
* Familiarizarse con Linux y su consola, el entorno de desarrollo y el repositorio.
* Aprender a utilizar las Commons, principalmente las funciones para listas, archivos de 
  configuración y logs.
* Definir el Protocolo de Comunicación.
* Comenzar el desarrollo de los módulos y sus conexiones.
```

### Documentacion 📖
```
* Tutoriales de “Cómo arrancar” de la materia: https://docs.utnso.com.ar/primeros-pasos
* SO Commons Library - https://github.com/sisoputnfrba/so-commons-library 
* Git para el Trabajo Práctico - https://docs.utnso.com.ar/guias/consola/git
* Guía de Punteros en C - https://docs.utnso.com.ar/guias/programacion/punteros
* Guía de Sockets - https://docs.utnso.com.ar/guias/linux/sockets
```
