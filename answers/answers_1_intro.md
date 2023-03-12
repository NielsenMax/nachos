# Respuestas

## 1. ¿Por que se prefiere emular una CPU en vez de utilizar directamente la CPU existente?
    Se decide emular un CPU ya que se usa una arquitectura mas simple de base, esta es la del microprocesador MIPS.
## 2. ¿Cuanta memoria tiene la maquina simulada para Nachos?
    4096 bytes
## 3. ¿Que modificarıa para cambiar la cantidad de memoria?
    Modificando la cantidad de paginas fisicas (NUM_PHYS_PAGES) o el tamano de estas (PAGE_SIZE) en machine/mmu.hh
## 4. ¿De que tama˜no es un disco?
    131076 bytes
## 5. ¿Cuantas instrucciones de MIPS simula Nachos?
    60 instrucciones
## 6. ¿En que archivos esta definida la funcion main? ¿En que archivo esta definida la funcion main del ejecutable nachos del directorio userprog?
    La funcion main esta definida en los siguientes archivos:
    - code/bin/coff2flat.c
    - code/bin/coff2noff.c
    - code/bin/disasm.c
    - code/bin/out.c
    - code/bin/readnoff.c
    - code/bin/fuse/nachosfuse.c
    - code/threads/main.cc
    - code/userland/echo.c
    - code/userland/filetest.c
    - code/userland/halt.c
    - code/userland/matmul.c
    - code/userland/shell.c
    - code/userland/sort.c
    - code/userland/tiny_shell.c
    - code/userland/touch.c
    <!-- TODO second part -->
## 7. Nombre los archivos fuente en los que figuran las funciones y metodos llamados por el main de Nachos al ejecutarlo en el directorio threads (hasta dos niveles de profundidad). Por ejemplo: main llama a Initialize, que esta en tal archivo; e Initialize llama a ASSERT, que esta en tal otro archivo.
    - Initialize: threads/system.cc
    - DEBUG: code/lib/debug.cc
    - SysInfo:
    - PrintVersion:
    - ThreadTest:
    - Interrupt::Halt:
    - ASSERT:
    - StartProcess:
    - ConsoleTest:
    <!-- TODO Finish this nonesense -->
## 8. ¿Que efecto hacen las macros ASSERT y DEBUG definidas en lib/utility.hh?
    ASSERT: Su funcion es validar una condicion y abortar el programa si esta no se cumple. Se usan a lo largo del proyecto para explicitar invariantes
    DEBUG: Se usa para escribir al stderr informacion util a la hora de debuggear. Esta solo se escribe si la flag pasada como parametro se encuentra presente 
## 9. Comente el efecto de las distintas banderas de depuraci´n.
    Las banderas de depuracion se usan para determinar sobre que componentes del projecto se quieren tener mensajes de depuracion. 
    Estas son las siguientes:
    - f: filesystem
    - d: disco
    - i: interrupciones
    - x: Overflow de ticks
    - m: machine
    - a: lectura y escritura de memoria
    - n: network
    - p: procesos
    - t: threads
    - e: programas de usuario
    - +: todas las anteriores
## 10. ¿Donde estan definidas las constantes USER_PROGRAM, FILESYS_NEEDED, FILESYS_STUB y NETWORK?
    En los makefiles de algunas de los componentes del projecto.
## 11. ¿Que lınea de comandos soporta Nachos? ¿Que efecto hace la opcion -rs?
    Nachos soporta los siguientes comandos:
    - d: Produce que ciertos mensajes de depuracion sean mostrados
    - do: Habilita las opciones de los mensajes de depuracion
    - p: Habilita la multitarea preventiva en los threads del nucleo
    - rs: Produce que la funcion Yield ocurra de manera aleatoria pero repetible
    - z: Escribe informacion de la version y copyright
    - tt: ejecuta test del sistema de threads
    - s: los programas de usario se ejecutan de a una instruccion por vez
    - x: ejecuta un programa de usuario
    - tc: Comprueba la consola
    - f: formatea el disco fisico
    - cp: copia un archivo de UNIX a Nachos
    - pr: escribe el contenido de un archivo de Nachos al stdout
    - rm: borra un archivo de Nachos del sistema de archivos
    - ls: lista el contenido de un directorio
    - D: escribe el contenido de todo el sistema de archivos
    - c: comprueba la integridad del sistema de archivos
    - tf: realiza una prueba del rendimiento del sistema de archivos
    - n: establece la fiabilidad de la red
    - id: establece el host id de la maquina
    - tn: ejecuta un test del sistema de red de Nachos.

## 12. Al ejecutar nachos -i, se obtiene informacion del sistema. Sin embargo esta incompleta. Modifique el codigo para que se muestren los datos que faltan.
    Completado en threads/sys_info.cc
## 13. ¿Cual es la diferencia entre las clases List y SynchList?
    List implementa una lista enlazada simple, que tiene funciones para insertar y remover en orden basandose en una prioridad. SynchList implementa un sistema de locks sobre List para volverla thread-safe.

## 14. Modifique el ejemplo del directorio threads para que se generen 5 hilos en lugar de 2.
    Completado

## 15. Modifique el caso de prueba para que estos cinco hilos utilicen un semaforo inicializado en 3. Esto debe ocurrir solo si se define la macro de compilacion SEMAPHORE TEST.
    Completado

## 16. Agregue al caso anterior una lınea de depuracion que diga cuando cada hilo hace un P() y cuando un V(). La salida debe verse por pantalla solamente si se activa la bandera de depuracion correspondiente.
    Completado

## 17. En threads se provee un caso de prueba que implementa el jardın ornamental. Sinembargo, el resultado es erroneo. Corrıjalo de forma que se mantengan los cambios de contexto, sin agregar nuevas variables.
## 18. Replique el jardın ornamental en un nuevo caso de prueba. Revierta la solucion anterior y solucione el problema usando semaforos esta vez.