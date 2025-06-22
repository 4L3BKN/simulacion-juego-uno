# Simulación del Juego UNO en C++ con Procesos y Memoria Compartida

Este proyecto implementa una simulación del juego de cartas UNO utilizando programación concurrente en C++. La coordinación entre jugadores se logra mediante procesos (`fork`) y memoria compartida con `mmap`.

## Créditos 

Fue desarrollado en pareja como parte de una asignatura universitaria. 

Mi aporte incluyó:
 - Modelado y creación del mazo de cartas.
 - Implementación de la memoria compartida con `mmap`.
 - Reparto inicial de cartas.
 - Gestión de turnos y sincronización entre procesos.
 - Lógica general del flujo del juego.

Mi compañero colaboró en parte de la lógica de cartas especiales(+2, +4, bloqueo, reversa).

## Funcionalidades

- Simulación de una partida de UNO con 4 jugadores:
  - 1 jugador humano (desde consola).
  - 3 jugadores automáticos (procesos hijos).
- Gestión de turnos usando una varible compartida con `mmap`.
- Lógica de juego con reglas básicas y control concurrente.

## Tecnologías utilizadas

- **C++**
- Procesos con `fork()`
- Memoria compartida con `mmap`
- STL: `vector`, `string`, `random`, `algorithm`, etc.

## Cómo compilar y ejecutar

```
g++ main.cpp -o uno 
./uno

```
Asegurarse de ejecutrar en un entorno compatible con `mmap` (Linux/macOS).

## Notas

- El propósito principal es aplicar conceptos de concurrencia y procesos.
- No cuenta con interfaz gráfica, más alla de la interacción por consola mediante `cout`.
