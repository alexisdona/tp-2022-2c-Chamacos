# tp-2022-2c-ChamacOS

LINK REPOSITORIO:
[https://github.com/sisoputnfrba/tp-2022-2c-ChamacOS] 

LINK ENUNCIADO:
[https://docs.google.com/document/d/1xYmkJXRRddM51fQZfxr3CEuhNtFCWe5YU7hhvsUnTtg/edit]

LINK PRUEBAS:
[https://docs.google.com/document/d/1qxF4h9dkL5L6X6P8qts997wTC1qLxIg6ElZzlU1Xgi4/edit]

---
1. Configurar la VM con el adapter de la placa de red
2. obtener la IP de clase C de la VM (ifconfig ) es del tipo 192.168.1.2
3. Conectarse con putty por ssh a la VM a la IP y puerto 22
4. Bajar las commons por consola de putty en la VM https://github.com/sisoputnfrba/so-commons-library.git
5. bajar el repo del TP por consola de putty en la VM https://github.com/sisoputnfrba/tp-2022-2c-ChamacOS.git
   Token alexis: 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
6. Actualizar communication.config con la IP de cada VM que vamos a usar
7. Instalar las commons con make install 
8. Actualizar communication.config con la IP de cada modulo y pushear a main
/** COMPILACION DE TP **/

9. Parado dentro de shared/src gcc -Wall -c shared_utils.c communication.c translation_utils.c -lcommons -lpthread
9. Parado dentro consola/src Compilar consola: gcc -Wall consola.c -o "consola" ../../shared/src/shared_utils.o ../../shared/src/communication.o ../../shared/src/translation_utils.o -lcommons
10. Parado dentro de kernel/src Compilar kernel: gcc -Wall kernel.c -o "kernel" ../../shared/src/shared_utils.o ../../shared/src/communication.o ../../shared/src/translation_utils.o -lcommons -lpthread
11. Parado dentro de cpu/src Compliar cpu: gcc -Wall cpu.c -o "cpu" ../../shared/src/shared_utils.o ../../shared/src/communication.o ../../shared/src/translation_utils.o -lcommons -lpthread
12. parado dentro de memoria/src Compilar memoria: gcc -Wall memoria.c -o "memoria" ../../shared/src/shared_utils.o ../../shared/src/communication.o ../../shared/src/translation_utils.o -lcommons -lpthread



