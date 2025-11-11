# TP2 - Sistema Operativo x64

Sistema operativo de 64 bits desarrollado en C y Assembly para arquitectura x86-64. Implementa gestión de procesos con scheduling por prioridades, sincronización mediante semáforos, comunicación entre procesos vía pipes, y dos estrategias de administración de memoria dinámica.

## Integrantes

| Nombre               | Padrón | Email                     |
| -------------------- | ------ | ------------------------- |
| Francisco Costa      | 65202  | frcosta@itba.edu.ar       |
| Román Salerno        | 65145  | rsalerno@itba.edu.ar      |
| Tiago Heras          | 65627  | theras@itba.edu.ar        |

---

## Índice

- [Compilación y Ejecución](#compilación-y-ejecución)
- [Comandos de la Shell](#comandos-de-la-shell)
- [Características del Kernel](#características-del-kernel)
- [Tests Integrados](#tests-integrados)
- [Operador de Background](#operador-de-background)
- [Uso de Pipes](#uso-de-pipes)
- [Atajos de Teclado](#atajos-de-teclado)
- [Ejemplos Prácticos](#ejemplos-prácticos)
- [Decisiones de Implementación](#decisiones-de-implementación)
- [Limitaciones](#limitaciones)
- [Uso de Inteligencia Artificial](#uso-de-inteligencia-artificial)
- [Referencias](#referencias)

---

## Compilación y Ejecución

### Requisitos Previos

- **Docker** instalado y en ejecución
- **QEMU** (para ejecutar el sistema operativo)

### Compilar el Proyecto

El sistema soporta dos implementaciones de memory manager que se seleccionan en tiempo de compilación:

```bash
# Compilar con Page List Memory Manager (por defecto)
./compile.sh

# Compilar con Buddy System Memory Manager
./compile.sh buddy
```

### Ejecutar el Sistema

```bash
./run.sh
```

Esto abrirá una ventana de QEMU con el sistema operativo iniciado.

**Nota para usuarios de Linux:** El script `run.sh` está configurado para macOS. En Linux, modificar la opción de audio de `-audiodev coreaudio` a `-audiodev pa` o comentar la línea completa si no se requiere audio.

### Comandos Make

```bash
make all      # Compila bootloader, kernel, userland e imagen con el Page List allocator
make buddy    # Compila todo con Buddy System
make clean    # Elimina archivos compilados
```

---

## Comandos de la Shell

La shell soporta dos tipos de comandos: **built-ins** que se ejecutan en el contexto de la shell, y **comandos de proceso** que crean nuevos procesos.

### Comandos Built-in

| Comando | Descripción | Uso |
|---------|-------------|-----|
| `help` | Muestra lista de comandos disponibles | `help` |
| `clear` | Limpia la pantalla | `clear` |
| `exit` | Cierra la shell | `exit` |
| `ps` | Lista todos los procesos con su estado | `ps` |
| `kill <pid>` | Termina un proceso específico | `kill 5` |
| `block <pid>` | Alterna un proceso entre READY y BLOCKED | `block 7` |
| `nice <pid> <prio>` | Cambia la prioridad de un proceso (0-3) | `nice 7 3` |
| `mem` | Muestra información del memory manager | `mem` |
| `time` | Muestra la hora actual del sistema | `time` |
| `mvar <w> <r>` | Inicia el problema productor-consumidor | `mvar 2 3` |

### Comandos que Crean Procesos

Estos comandos pueden usarse en pipes y soportan ejecución en background con `&`.

| Comando | Descripción | Ejemplo |
|---------|-------------|---------|
| `cat` | Lee de stdin y escribe a stdout | `cat` |
| `echo <texto>` | Imprime argumentos en stdout | `echo Hola Mundo` |
| `filter` | Filtra vocales de stdin | `cat \| filter` |
| `wc` | Cuenta líneas de stdin | `cat \| wc` |
| `loop [seg]` | Imprime saludo cada N segundos | `loop 5 &` |
| `test_process` | Test de creación/destrucción de procesos | `test_process` |
| `test_prio <iter>` | Test de prioridades del scheduler | `test_prio 1000000` |
| `test_sync <n> <0\|1>` | Test de sincronización con/sin semáforos | `test_sync 1000 1` |

### Comandos de Demostración de Pipes

| Comando | Descripción |
|---------|-------------|
| `pipe_demo` | Ejemplo de productor/consumidor con pipe |
| `pipe_eof` | Demuestra comportamiento de EOF en pipes |
| `pipe_broken` | Demuestra broken pipe cuando no hay lectores |
| `pipe_sync` | Muestra comportamiento de bloqueo en pipes |
| `pipe_stress` | Stress test con múltiples lectores/escritores |

---

## Características del Kernel

### Gestión de Procesos

- **Creación y destrucción**: Syscalls `createProcess()`, `exit()` y `kill()`
- **PIDs únicos**: Asignación secuencial con reciclaje
- **Estados**: READY, RUNNING, BLOCKED, TERMINATED
- **Proceso Init** (PID 1): Proceso idle del sistema
- **Stacks independientes**: Cada proceso tiene su propio stack
- **Foreground/Background**: Procesos pueden ejecutarse en primer o segundo plano

### Scheduler

- **Algoritmo**: Round-robin multinivel con 4 niveles de prioridad (0-3, siendo 3 la máxima)
- **Quantum fijo**: Todos los procesos tienen el mismo quantum
- **Aging**: Procesos que esperan mucho tiempo reciben boost temporal de prioridad
- **Preemption**: Por timer tick y yield voluntario
- **Syscalls**: `yield()`, `sleep()`, `getpid()`, `nice()`

### Semáforos

- **Tipo**: Semáforos contadores con nombres
- **Operaciones**: `semOpen()`, `semClose()`, `semWait()`, `semPost()`
- **Bloqueo**: Procesos que hacen wait en semáforo con valor 0 se bloquean
- **Múltiples instancias**: Soporta múltiples semáforos simultáneos con nombres únicos
- **Reference Counting**: Control automático de referencias para gestión de ciclo de vida
- **Gestión automática de recursos**: Cuando un proceso termina o es matado, todos los semáforos que tenía abiertos se cierran automáticamente, previniendo leaks de recursos. Cada proceso mantiene un registro de sus semáforos abiertos (máximo 8) y el kernel los libera al terminar el proceso

### Pipes

- **Implementación**: Buffer circular de 4KB por pipe
- **Bloqueo automático**: El escritor se bloquea si el buffer está lleno, el lector si está vacío
- **EOF**: Se envía cuando todos los escritores cierran el pipe
- **File descriptors**: Los pipes se asignan como FDs (0: stdin, 1: stdout)
- **Sintaxis shell**: `comando1 | comando2`

### Memory Managers

Dos implementaciones intercambiables mediante flag de compilación:

#### Page List Memory Manager
- **Estrategia**: Lista enlazada de páginas de 1KB que se asignan/reutilizan como unidades atómicas
- **Ventaja**: Sobrecosto mínimo y asignaciones con tiempo constante manteniendo el seguimiento a nivel de página
- **Desventaja**: Fragmentación interna para pedidos más pequeños que una página completa
- **Build**: Es la opción por defecto compilada con `./compile.sh` o `make all`

#### Buddy System
- **Estrategia**: Bloques en potencias de 2 con división/fusión
- **Ventaja**: Reduce fragmentación, fusión eficiente
- **Desventaja**: Fragmentación interna (desperdicio en bloques grandes)

**Syscalls**: `sys_alloc_memory()`, `sys_free_memory()`, `sys_get_memory_state()`

---

## Tests Integrados

### test_process

Crea múltiples procesos, los bloquea/desbloquea aleatoriamente y finalmente los mata.

```bash
test_process
```

**Propósito**: Verificar robustez del scheduler y gestión de estados de procesos.

### test_prio

Ejecuta procesos con diferentes prioridades y mide cuánto avanzan en tiempo fijo.

```bash
test_prio <iteraciones>
```

**Propósito**: Validar que el scheduler respete las prioridades asignadas.

### test_sync

Crea pares de procesos que incrementan/decrementan una variable global compartida.

```bash
test_sync <iteraciones> <0|1>
```

- `1`: Con semáforo (resultado determinístico)
- `0`: Sin semáforo (race conditions observables)

**Propósito**: Demostrar necesidad de sincronización.

### mvar - Productor-Consumidor

Implementación del problema clásico de sincronización con múltiples escritores y lectores compartiendo un bounded buffer de 3 slots. Utiliza 4 semáforos para coordinar el acceso: `empty`, `full`, `write_mutex` y `read_mutex`.

```bash
mvar <num_writers> <num_readers>
```

**Ejemplo:**
```bash
mvar 2 3  # 2 escritores, 3 lectores
```

Cada escritor produce una letra única (A, B, C...) y cada lector la consume mostrándola en un color fijo asignado aleatoriamente. La prioridad de los procesos afecta la frecuencia con que aparecen las letras y colores en pantalla.

---

## Operador de Background

### Sintaxis

```bash
comando [argumentos] &
```

### Comportamiento

El operador `&` al final de un comando hace que se ejecute en **segundo plano**, permitiendo que la shell continúe aceptando comandos sin esperar a que termine.

### Reglas

- El `&` debe aparecer **al final** del comando, separado por espacio
- Solo funciona con **comandos de proceso** (no built-ins)
- Si se combina con pipes, el `&` se ignora
- Los procesos en background **no pueden leer de stdin** (reciben EOF inmediatamente)
- La shell imprime el PID del proceso creado

### Ejemplos

```bash
# Ejecutar loop en background
loop 10 &

# Crear múltiples procesos en background
loop 5 &
loop 8 &
ps

# Ver procesos corriendo
ps

# Matar proceso específico
kill <pid>
```

### Diferencia con Foreground

| Aspecto | Foreground | Background |
|---------|------------|------------|
| Shell bloqueada | Sí | No |
| Puede leer stdin | Sí | No (EOF) |
| La shell espera con wait() | Sí | No |
| Símbolo | (ninguno) | `&` |

---

## Uso de Pipes

### Sintaxis Básica

```bash
comando1 | comando2
```

### Ejemplos

```bash
# Contar líneas
cat | wc

# Filtrar vocales
echo "hola mundo" | filter

# Demostración completa
pipe_demo
```

### Limitaciones

- Solo se soporta un nivel de pipe (A | B)
- No se soportan cadenas (A | B | C)
- Ambos comandos deben ser de tipo proceso
- No se pueden usar built-ins en pipes

---

## Atajos de Teclado

La shell soporta los siguientes atajos:

| Atajo | Función |
|-------|---------|
| `Enter` | Ejecuta el comando actual |
| `Backspace` | Borra el carácter anterior |
| `Ctrl + C` | Interrumpe el proceso en foreground (si el kernel lo soporta) |
| `Ctrl + D` | Envía EOF |

**Notas:**
- Las flechas solo funcionan cuando hay historial disponible
- Los caracteres especiales y mayúsculas se obtienen con Shift

---

## Ejemplos Prácticos

### Monitoreo de Procesos

```bash
# Listar procesos
ps

# Crear proceso en background
loop 5 &

# Ver el nuevo proceso
ps

# Matar proceso
kill <pid>
```

### Testing de Scheduler

```bash
# Ejecutar test de prioridades
test_prio 1000000

# Mientras corre, verificar estado
ps

# Observar que procesos con más prioridad avanzan más
```

### Sincronización

```bash
# Sin semáforo (incorrecto)
test_sync 1000 0
# Observar valor final incorrecto

# Con semáforo (correcto)
test_sync 1000 1
# Observar valor final correcto (0)
```

### Memory Manager

```bash
# Ver estado inicial
mem

# Ejecutar test
test_mm 2048

# Ver cambios en memoria
mem
```

### Uso de Background y Pipes

```bash
# Múltiples procesos en background
loop 3 &
loop 7 &
loop 10 &

# Pipe simple
cat | filter

# Ver todos los procesos
ps
```

---

## Decisiones de Implementación

### Scheduler

**Decisión**: Round-robin multinivel con aging

**Razones**:
- Implementación directa y eficiente
- Previene starvation mediante envejecimiento
- Balance adecuado entre equidad y tiempo de respuesta

**Trade-off**: No optimizado para sistemas de tiempo real

### Pipes

**Decisión**: Buffer circular de tamaño fijo (1KB)

**Razones**:
- Tamaño suficiente para la mayoría de casos prácticos
- Implementación eficiente con índices circulares
- Simplicidad en la gestión de memoria

**Trade-off**: No se ajusta dinámicamente según la carga

### Memory Manager

**Decisión**: Dos implementaciones intercambiables en compile-time

**Razones**:
- Permite comparación directa de rendimiento
- Código más simple sin lógica de selección en runtime
- Facilita testing de cada estrategia por separado

**Trade-off**: Requiere recompilación para cambiar de estrategia

### Background Processes

**Decisión**: Procesos background no pueden leer stdin

**Razones**:
- Evita bloquear la interacción del usuario
- Previene condiciones de carrera en la entrada
- Comportamiento predecible y consistente

**Trade-off**: Limita casos de uso interactivos en background

### Gestión Automática de Semáforos

**Decisión**: Tracking automático de semáforos por proceso con liberación automática

**Implementación**:
- Cada proceso mantiene un array de hasta 8 semáforos abiertos
- Al hacer `semCreate()` o `semOpen()`, el semáforo se registra automáticamente en el proceso
- Al hacer `semClose()`, se desregistra del proceso
- Cuando un proceso termina (`exit`) o es matado (`kill`), el kernel cierra automáticamente todos los semáforos registrados

**Razones**:
- Previene leaks de semáforos cuando procesos mueren inesperadamente
- Transparente para el código de usuario (no requiere cambios en programas existentes)
- Similar al modelo de file descriptors (automático y robusto)
- Garantiza liberación de recursos incluso si un proceso crashea

**Trade-off**: Límite de 8 semáforos simultáneos por proceso (suficiente para casos prácticos)

---

## Limitaciones

### Del Sistema

1. **Memoria estática**: El kernel no puede expandir su heap dinámicamente
2. **Límite de procesos**: Definido en compile-time (típicamente 64)
3. **Pipes de un solo nivel**: No se soportan cadenas `A | B | C`
4. **Sin redirecciones**: No hay soporte para `>`, `>>`, `<`
5. **Semáforos limitados**: Número máximo definido en compile-time
6. **Sin detección de deadlock**: El sistema no detecta ni previene deadlocks

### De la Shell

1. **Sin job control avanzado**: No hay `fg`, `bg`, `jobs`
2. **Sin variables de entorno**: No se soportan variables
3. **Sin wildcards**: No hay expansión de `*` o `?`
4. **Sin autocompletado**: No hay completion de comandos o paths

---

## Uso de Inteligencia Artificial

Durante el desarrollo de este proyecto se utilizó **GitHub Copilot** como herramienta de asistencia para optimizar el proceso de programación y mejorar la calidad del código.

### Aplicaciones Específicas

#### 1. Autocompletado Inteligente
Copilot fue utilizado para acelerar la escritura de código repetitivo, especialmente en:
- Implementación de syscalls con patrones similares
- Estructuras de datos con múltiples campos
- Bucles de iteración sobre arreglos de procesos
- Validaciones de parámetros en funciones del kernel

#### 2. Sugerencias de Nombres
La herramienta asistió en la selección de nombres descriptivos y consistentes para:
- Variables de estado del scheduler
- Funciones auxiliares de gestión de memoria
- Constantes simbólicas del sistema
- Campos de estructuras de sincronización

#### 3. Documentación
Copilot generó esqueletos de comentarios y documentación que luego fueron revisados y expandidos:
- Headers de funciones con parámetros y valores de retorno
- Comentarios explicativos de secciones críticas
- Descripciones de algoritmos complejos (ej: búsqueda en buddy system)

#### 4. Detección de Errores
Durante el desarrollo, Copilot sugirió correcciones para:
- Chequeos de NULL pointers faltantes
- Validación de límites de arreglos
- Liberación de recursos (semáforos, memoria)
- Condiciones de carrera potenciales

---

## Referencias

### Código Base

Este proyecto se desarrolló a partir del template "x64 Bare Bones" proporcionado por la cátedra de Sistemas Operativos (72.08) del ITBA.

### Componentes de Terceros

- **Pure64 Bootloader**: Bootloader de 64 bits open source
  - Licencia: BSD
  - Ubicación: `Bootloader/Pure64/`

### Material de Consulta

- OSDev Wiki: https://wiki.osdev.org/
- Intel® 64 and IA-32 Architectures Software Developer Manuals
- "Operating Systems: Three Easy Pieces" - Remzi H. Arpaci-Dusseau
- Documentación de QEMU
- Documentación de GDB

---

## Estructura del Proyecto

```
TPE-SO-2/
├── Bootloader/           # Pure64 bootloader
├── Kernel/               # Código del kernel
│   ├── process.c         # Gestión de procesos
│   ├── scheduler.c       # Scheduling
│   ├── semaphore.c       # Semáforos
│   ├── pipe.c            # Pipes
│   ├── ipc.c             # IPC
│   ├── MemoryManager.c   # Page List allocator
│   ├── buddyMemoryManager.c  # Buddy system
│   └── drivers/          # Drivers (teclado, video, sonido, tiempo)
├── Userland/
│   ├── Shell/            # Shell interactiva
│   │   ├── shell.c
│   │   ├── mvar.c        # Problema productor-consumidor
│   │   ├── cat.c
│   │   ├── wc.c
│   │   ├── filter.c
│   │   └── echo.c
│   ├── test/             # Tests del sistema
│   ├── libc/             # Biblioteca C
│   └── libsys/           # Syscall wrappers
└── Image/                # Imagen de disco generada
```

---

## Licencia

Ver archivo `License.txt` para más información.

---

**Trabajo Práctico 2 - Sistemas Operativos (72.08)**
Instituto Tecnológico de Buenos Aires (ITBA)
Primer Cuatrimestre 2025
