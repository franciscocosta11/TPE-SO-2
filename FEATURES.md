# Características Implementadas - TPE Sistema Operativo

**Proyecto**: TPE Arquitectura de las Computadoras (72.08)
**Fecha**: Noviembre 2024
**Equipo**: Francisco Costa, Roman Salerno, Tiago Heras

---

## Arquitectura del Sistema

### Kernel (Bare-metal x86-64)
- **Modelo de memoria**: Flat memory model sin paginación
- **Memory Manager**: Buddy allocator con coalescing
- **Scheduler**: Round-robin con prioridades (0-10)
- **Gestión de procesos**: PCB (Process Control Block), context switching
- **Sincronización**: Semáforos con spinlocks atómicos (_xchg)
- **IPC**: Pipes con abstracción de File Operations
- **Excepciones**: Division by zero, Invalid opcode, Page fault

### Syscalls Implementadas
```c
// Process Management
int64_t sys_create_process(void (*entry)(uint64_t, char **), void *arg, char *name, int foreground, int priority);
int64_t sys_kill(int pid);
int64_t sys_block(int pid);
int64_t sys_unblock(int pid);
int64_t sys_waitpid(int pid);
int64_t sys_getpid();
int64_t sys_yield();
int64_t sys_nice(int pid, int newPriority);
int64_t sys_ps(ProcessInfo *buffer, int max);

// Memory Management
void* sys_alloc_memory(uint64_t size);
int sys_free_memory(void *ptr);
int sys_mem_state(MemoryState *state);

// IPC
int sys_pipe(int fds[2]);
int sys_read(int fd, char *buf, int count);
int sys_write(int fd, const char *buf, int count);
int sys_close(int fd);
int sys_dup2(int oldfd, int newfd);

// Semaphores
int sys_sem_create(const char *name, int initialValue);
int sys_sem_open(const char *name);
int sys_sem_wait(int semId);
int sys_sem_post(int semId);
int sys_sem_close(int semId);

// Time
uint64_t sys_seconds_elapsed();
void sys_sleep(uint32_t ms);

// Misc
void sys_clear_screen();
void sys_beep();
```

---

## Comandos de Shell Disponibles

### Gestión de Procesos
- **`ps`** - Lista todos los procesos activos con PID, estado, prioridad, nombre
- **`kill <pid>`** - Termina un proceso por PID
- **`block <pid>`** - Alterna un proceso entre BLOCKED y READY
- **`nice <pid> <priority>`** - Cambia la prioridad de un proceso (0-10)

### Comandos Básicos
- **`clear`** - Limpia la pantalla
- **`echo <text>`** - Imprime texto
- **`help`** - Lista todos los comandos disponibles
- **`man <command>`** - Muestra descripción de un comando
- **`history`** - Muestra historial de comandos (últimos 10)
- **`time`** - Muestra la hora actual
- **`font increase|decrease`** - Ajusta tamaño de fuente
- **`exit [code]`** - Sale del shell con código de salida

### Debugging y Excepciones
- **`divzero`** - Genera excepción de división por cero
- **`invop`** - Genera excepción de opcode inválido
- **`regs`** - Muestra snapshot de registros (tras excepción)
- **`mem`** - Muestra uso de memoria del kernel

### Procesos de Ejemplo
- **`loop [seconds]`** - Imprime PID cada N segundos (default: 1)
- **`sleep2`** - Proceso foreground que duerme 2 segundos
- **`print3`** - Imprime 3 líneas y sale
- **`cat`** - Echo de stdin a stdout (hasta EOF)

---

## Tests Implementados

### 1. Test de Memory Manager (`test_mm`)
```bash
test_mm <max_bytes>
```
- Stress test del memory manager con bloques aleatorios
- Verifica allocación/liberación sin memory leaks
- Valida coalescing del buddy allocator
- **Estado**: ✅ Funcionando

### 2. Test de Prioridades (`test_prio`)
```bash
test_prio <max_iterations>
```
- Verifica que procesos con mayor prioridad ejecutan más
- Crea procesos con diferentes prioridades
- Mide cuántas veces ejecuta cada uno
- **Estado**: ✅ Funcionando

### 3. Test de Sincronización (`test_sync`)
```bash
test_sync <n> <use_sem>
# n = número de incrementos/decrementos
# use_sem: 0=sin sync (race condition), 1=con semáforos
```
- Demuestra race conditions sin sincronización
- Valida corrección con semáforos
- Usa 6 procesos: 2 que incrementan +2, 2 que incrementan +1, 2 que decrementan -2, 2 que decrementan -1
- **Resultado esperado**: valor final = 0 (con semáforos), != 0 (sin semáforos)
- **Estado**: ✅ Funcionando

### 4. Test de Procesos (`test_process`)
```bash
test_process <max_processes>
```
- Crea, bloquea y mata procesos aleatoriamente
- Stress test del scheduler y process manager
- **Estado**: ✅ Funcionando

---

## IPC y Pipes

### Implementación de Pipes
- **Buffer circular**: 4096 bytes
- **Bloqueo**: Writer bloquea si pipe lleno, reader si vacío
- **EOF**: Reader recibe EOF cuando todos los writers cierran
- **Broken pipe**: Writer falla si no hay readers
- **File descriptors**: 0=stdin, 1=stdout, 2=stderr
- **Redirección**: Soporte de `|` para pipelines

### Comandos de Demostración

#### `pipe_demo`
Demostración básica de pipe entre dos procesos:
```
Writer → [PIPE] → Reader
```

#### `pipe_eof`
Muestra EOF cuando el writer cierra:
```
Writer escribe datos → cierra → Reader recibe EOF
```

#### `pipe_broken`
Muestra broken pipe cuando no hay readers:
```
Reader cierra → Writer intenta escribir → error EPIPE
```

#### `pipe_sync`
Test de sincronización de pipes:
```
Writer bloquea cuando pipe lleno
Reader bloquea cuando pipe vacío
```

#### `pipe_stress [numWriters] [numReaders]`
Stress test con múltiples writers/readers concurrentes
```bash
pipe_stress 4 2  # 4 writers, 2 readers
```

### Soporte de Pipelines
El shell soporta pipelines con `|`:
```bash
loop 1 | cat        # loop escribe a pipe, cat lee de pipe
echo hello | cat    # echo escribe "hello", cat lo imprime
```

---

## Implementación de Semáforos

### Características
- **Tipo**: Semáforos nombrados (estilo POSIX)
- **Operaciones**: create, open, wait (P), post (V), close
- **Capacidad**: Hasta 16 semáforos simultáneos
- **Reference counting**: Auto-destrucción cuando ref_count = 0
- **Sincronización interna**: Spinlocks con _xchg atómico

### API de Semáforos
```c
// Crear semáforo (si no existe) o abrirlo (si existe)
int sem_id = sys_sem_create("mi_sem", 1);

// Abrir semáforo existente
int sem_id = sys_sem_open("mi_sem");

// P (wait) - decrementa, bloquea si value < 0
sys_sem_wait(sem_id);

// V (post) - incrementa, desbloquea un proceso en espera
sys_sem_post(sem_id);

// Cerrar semáforo (decrementa ref_count)
sys_sem_close(sem_id);
```

### Validaciones Implementadas
- ✅ NULL checks en sys_sem_create/open
- ✅ Validación de semId en todas las operaciones
- ✅ Protección con spinlocks para operaciones atómicas
- ✅ Cola de espera cuando value < 0

---

## Gestión de Memoria

### Buddy Allocator
- **Algoritmo**: Buddy system con power-of-2 sizes
- **Coalescing**: Automático al liberar
- **Free lists**: Una por cada tamaño (2^k bytes)
- **Fragmentación**: Minimizada con coalescing
- **Validaciones**: NULL checks, double-free detection

### Comando `mem`
Muestra información de memoria:
```
Total memory: X MB
Used memory: Y MB
Free memory: Z MB
Largest free block: W KB
```

---

## Características Adicionales

### Scheduler
- **Algoritmo**: Round-robin con prioridades
- **Prioridades**: 0 (más baja) a 10 (más alta)
- **Quantum**: Configurable (default: 10ms)
- **Estados**: READY, RUNNING, BLOCKED, ZOMBIE

### Excepciones
- **Division by zero**: Captura y muestra snapshot de registros
- **Invalid opcode**: Ídem
- **Page fault**: Ídem
- **Snapshot**: Guardado de todos los registros generales + rip, rsp

### Drivers
- **Keyboard**: Soporte de teclas especiales (↑↓ para historial, Ctrl+C)
- **Video**: Modo texto VGA con scroll, colores ANSI 4-bit
- **Timer**: PIT para scheduling, `sys_seconds_elapsed()`
- **Sound**: Beeper con `sys_beep()`

### Shell Features
- **Historial**: Últimos 10 comandos (navegación con ↑↓)
- **Ctrl+C**: Mata proceso foreground
- **Background processes**: Soporte de procesos en background
- **Pipelines**: Soporte de `|` para conectar procesos
- **Exit codes**: Comandos retornan códigos de salida

---

## Calidad de Código

### Análisis Estático
- ✅ **PVS-Studio**: 0 warnings reales (2 falsos positivos)
- ✅ **Compilación**: Sin errores ni warnings
- ✅ **Validación de parámetros**: En todas las syscalls
- ✅ **Manejo de errores**: Robusto con códigos de error

### Testing
- ✅ Memory manager: test_mm funcionando
- ✅ Prioridades: test_prio funcionando
- ✅ Sincronización: test_sync funcionando (con y sin semáforos)
- ✅ Procesos: test_process funcionando
- ✅ Pipes: 5 tests de IPC funcionando

---

## Estructura del Proyecto

```
TPE-SO-2/
├── Bootloader/          # Bootloader x86-64
├── Kernel/              # Kernel bare-metal
│   ├── include/         # Headers del kernel
│   ├── MemoryManager.c  # Buddy allocator
│   ├── process.c        # Gestión de procesos
│   ├── scheduler.c      # Scheduler round-robin
│   ├── semaphore.c      # Implementación de semáforos
│   ├── pipe.c           # Implementación de pipes
│   └── syscalls.c       # System calls
├── Userland/            # Programas de usuario
│   ├── Shell/           # Shell con todos los comandos
│   ├── test/            # Test suite
│   ├── libc/            # Standard library mínima
│   └── libsys/          # Wrappers de syscalls
└── Image/               # Imagen booteable generada

```

---

## Compilación y Ejecución

### Requisitos
- Docker instalado y corriendo
- Make
- Bash

### Compilar y Ejecutar
```bash
./compile.sh    # Compila kernel + userland
./run.sh        # Ejecuta en QEMU
```

### Análisis Estático
```bash
./run_pvs_analysis.sh    # PVS-Studio (requiere instalación previa)
./run_cppcheck.sh        # Cppcheck (requiere: sudo apt install cppcheck)
```

---

## Estado del Proyecto

### Completado ✅
- [x] Gestión de procesos con scheduler
- [x] Memory manager (buddy allocator)
- [x] Semáforos nombrados
- [x] IPC con pipes
- [x] Shell funcional con 25+ comandos
- [x] Test suite completo (mm, prio, sync, process)
- [x] Pipelines en shell
- [x] Análisis estático (PVS-Studio)
- [x] Documentación de código
- [x] Manejo de excepciones

### Análisis de Calidad
- **Warnings de compilación**: 0 (2 falsos positivos verificados)
- **PVS-Studio**: 0 problemas reales
- **Tests**: 4/4 funcionando correctamente
- **Memory leaks**: 0 detectados

---

## Referencias

- **Manual de usuario**: [Google Docs](https://docs.google.com/document/d/1ZWmG98adobSHLwyexbFj743G0-Je5KMigvoO34VTcoc/edit?usp=sharing)
- **Informe del proyecto**: [Google Docs](https://docs.google.com/document/d/1RvDtHoayLrMMgk9ywLVE4wvfPCtpFVDpyeJib6fbQIc/edit?usp=sharing)
- **Análisis estático**: Ver [ANALISIS_ESTATICO_RESUMEN.md](ANALISIS_ESTATICO_RESUMEN.md)
- **Code review**: Ver [CODE_REVIEW.md](CODE_REVIEW.md)

---

**Conclusión**: El sistema operativo implementa todas las características requeridas para el TPE, con código de calidad profesional y testing exhaustivo.
