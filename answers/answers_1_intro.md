# Respuestas

## 1. ¿Cuanta memoria tiene la maquina simulada para Nachos?
    4096 bytes
## 2. ¿Como cambiarıa ese valor?
    Modificando la cantidad de paginas fisicas (NUM_PHYS_PAGES) o el tamano de estas (PAGE_SIZE) en machine/mmu.hh
## 3. ¿De que tama˜no es un disco?
    131076 bytes
## 4. ¿Cuantas instrucciones de MIPS simula Nachos?
    60
## 5. Explique el codigo que procesa la instruccion add.
    `Machine::ExecInstruction()`:
    suma los valores de rs y rt. Si tienen el mismo signo y este cambia al sumar, se hace un raise exception de overflow. Si no see guarda el valor de la suma en rd
## 6. Nombre los archivos fuente en los que figuran las funciones y metodos llamados por el main de Nachos al ejecutarlo en el directorio threads (hasta dos niveles de profundidad).
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
## 7. ¿Por que se prefiere emular una CPU en vez de utilizar directamente la CPU existente?
## 8. ¿Que efecto hacen las macros ASSERT y DEBUG definidas en lib/utility.hh?
## 9. Comente el efecto de las distintas banderas de depuracion.
## 10. ¿Donde estan definidas las constantes USER PROGRAM, FILESYS NEEDED, FILESYS STUB y NETWORK?
    En los makefiles
## 11. ¿Cual es la diferencia entre las clases List y SynchList?
## 12. ¿En que archivos esta definida la funcion main? ¿En que archivo esta definida la funcion main del ejecutable nachos del directorio userprog?
## 13. ¿Que lınea de comandos soporta Nachos? ¿Que efecto hace la opcion -rs?
## 14. Modifique el ejemplo del directorio threads para que se generen 5 hilos en lugar de 2.
## 15. Modifique el ejemplo para que estos cinco hilos utilicen un semaforo inicializado en 3. Esto debe ocurrir solo si se define la macro de compilacion SEMAPHORE TEST.
## 16. Agregue al ejemplo anterior una lınea de depuracion que diga cuando cada hilo hace un P() y cuando un V(). La salida debe verse por pantalla solamente si se activa la bandera de depuracion correspondiente.