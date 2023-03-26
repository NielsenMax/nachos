# Respuestas

## 1. ¿Por que se prefiere emular una CPU en vez de utilizar directamente la CPU existente?
    Se decide emular un CPU ya que se usa una arquitectura mas simple de base, esta es la del microprocesador MIPS. Con esto, logramos tener control de las características físicas del sistema sin tener que tocar el hardware subyacente. Además, facilita el aprendizaje de las tareas a realizar.
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

    El archivo main del directorio userprog está definido en code/threads/main.cc. 
## 7. Nombre los archivos fuente en los que figuran las funciones y metodos llamados por el main de Nachos al ejecutarlo en el directorio threads (hasta dos niveles de profundidad). Por ejemplo: main llama a Initialize, que esta en tal archivo; e Initialize llama a ASSERT, que esta en tal otro archivo.
    Main llama a:
    - Initialize, que está en threads/system.cc y llama a:
      - ASSERT, que está en lib/assert.hh
      - strcmp, que está en string.h
      - ParseDebugOpts, que está en threads/system.cc
      - SystemDep::RandomInit, que está en machine/system_dep.cc
      - atoi, que está en stdlib.h
      - atof, que está en stdlib.h
      - Debug::SetFlags, que está en lib/debug.cc
      - Debug::SetOpts, que está en lib/debug.cc
      - Interrupt::Interrupt, que está en machine/interrupt.cc
      - Scheduler::Scheduler, que está en threads/scheduler.cc
      - Timer::Timer, que está en machine/timer.cc
      - Thread::Thread, que está en threads/thread.cc
      - Thread::SetStatus, que está en threads/thread.cc
      - Interrupt::Enable, que está en machine/interrupt.cc
      - SystemDep::CallOnUserAbort, que está en machine/system_dep.cc
      - PreemptiveScheduler, que está en threads/preemptive.hh
      - PreemptiveScheduler::SetUp, que está en threads/preemptive.cc
      - Debugger, que está en userprog/debugger.hh
      - Machine::Machine, que está en machine/machine.cc
      - SetExceptionHandlers, que está en userprog/exception.cc
      - SynchDisk::SynchDisk, que está en filesys/synch_disk.cc
      - FileSystem::FileSystem, que está en filesys/file_system.hh
      - PostOffice::PostOffice, que está en network/post.cc

    - DEBUG, que está en lib/utility.hh
    - strcmp, que está en string.h
    - SysInfo, que está en threads/sys_info.cc y llama a:
      - print, que está en stdio.h
    - PrintVersion, que está en threads/main.cc y llama a:
      - print, que está en stdio.h
    - ThreadTest, que está en threads/thread_test.cc y llama a:
      - DEBUG, que está en lib/utility.hh
      - Choose, que está en threads/thread_test.cc
      - Run, que está en threads/thread_test.cc
    - Interrupt::Halt, que está en machine/interrupt.cc y llama a:
      - print, que está en stdio.h
      - Statistics::Print, que está en machine/statistics.cc
      - Cleanup, que está en threads/system.cc
    - ASSERT, que está definido en lib/assert.hh
    - StartProcess, que está definido en userprog/prog_test.cc y llama a:
      - ASSERT, que está definido en lib/assert.hh
      - Open, que está definido en filesys/file_system.hh
      - print, que está en stdio.h
      - AddressSpace::AddressSpace, que está en userprog/address_space.cc
      - AddressSpace::InitRegisters, que está en userprog/address_space.cc
      - AddressSpace::RestoreState, que está en userprog/address_space.cc
      - Machine::Run(), que está en machine/mips_sim.cc
      - ASSERT, que está definido en lib/assert.hh
    - ConsoleTest, que está definido en userprog/prog_test.cc y llama a:
      - Console::Console, que está en machine/console.cc
      - Semaphore::Semaphore, que está en threads/semaphore.cc
      - Semaphore::P, que está en threads/semaphore.cc
      - Console::GetChar, que está en machine/console.cc
      - Console::PutChar, que está en machine/console.cc
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
    Completado, permutar linea 26 y 27
## 18. Replique el jardın ornamental en un nuevo caso de prueba. Revierta la solucion anterior y solucione el problema usando semaforos esta vez.
    Completado