# Respuestas

## 5b. Modifique la implementación para solucionar o evitar en el caso de los “locks” y
## variables de condición el problema de inversión de prioridades.
## Explique (en un archivo de texto) por qué no puede hacerse lo mismo con los
## semáforos.

Dado que un lock solo puede ser liberado por el proceso que lo adquirió, podemos identificar su 
prioridad y, en consecuencia, dejar que otro proceso de mayor prioridad pueda adquirir el lock. No 
es el caso de los semáforos, en donde no es posible saber quién libera el lock ni definir su 
prioridad.