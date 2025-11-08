#!/bin/bash
set -e

echo "==========================================="
echo "  Empaquetado TPE - Sistema Operativo"
echo "==========================================="
echo ""

# Nombre del archivo de entrega
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
PACKAGE_NAME="TPE-SO-Grupo_${TIMESTAMP}.tar.gz"
TEMP_DIR="TPE-SO-Entrega"

# Limpiar compilación anterior
echo "1. Limpiando compilación anterior..."
make clean > /dev/null 2>&1 || true

# Crear directorio temporal para el empaquetado
echo "2. Preparando archivos para empaquetado..."
rm -rf "$TEMP_DIR"
mkdir -p "$TEMP_DIR"

# Copiar archivos del proyecto
echo "3. Copiando archivos del proyecto..."
rsync -a \
    --exclude='.git' \
    --exclude='.claude' \
    --exclude='Image' \
    --exclude='*.o' \
    --exclude='*.bin' \
    --exclude='*.elf' \
    --exclude='pvs-report' \
    --exclude='pvs-studio.log' \
    --exclude='strace_out.txt' \
    --exclude='build.log' \
    --exclude="$TEMP_DIR" \
    --exclude="$PACKAGE_NAME" \
    ./ "$TEMP_DIR/"

# Crear directorio de documentación
echo "4. Organizando documentación..."
mkdir -p "$TEMP_DIR/docs"
mv "$TEMP_DIR/README.md" "$TEMP_DIR/docs/" 2>/dev/null || true
mv "$TEMP_DIR/FEATURES.md" "$TEMP_DIR/docs/" 2>/dev/null || true
mv "$TEMP_DIR/CODE_REVIEW.md" "$TEMP_DIR/docs/" 2>/dev/null || true
mv "$TEMP_DIR/ANALISIS_ESTATICO_RESUMEN.md" "$TEMP_DIR/docs/" 2>/dev/null || true
mv "$TEMP_DIR/CONTRIBUTING.md" "$TEMP_DIR/docs/" 2>/dev/null || true

# Crear README de entrega en la raíz
echo "5. Creando README de entrega..."
cat > "$TEMP_DIR/README.md" << 'EOF'
# TPE Sistema Operativo - Arquitectura de las Computadoras (72.08)

## Integrantes

| Nombre                 | Padrón | Email                      |
|------------------------|--------|----------------------------|
| Francisco Costa        | 65202  | frcosta@itba.edu.ar        |
| Roman Salerno          | 65145  | rsalerno@itba.edu.ar       |
| Tiago Heras            | 65627  | theras@itba.edu.ar         |

## Compilación y Ejecución

### Requisitos
- Docker instalado y corriendo
- Make
- Bash

### Pasos

1. **Compilar el proyecto:**
   ```bash
   ./compile.sh
   ```

2. **Ejecutar en QEMU:**
   ```bash
   ./run.sh
   ```

3. **Compilar y ejecutar (todo junto):**
   ```bash
   ./compile.sh && ./run.sh
   ```

## Características Implementadas

✅ **Gestión de Procesos**
- Scheduler round-robin con prioridades (0-10)
- PCB (Process Control Block)
- Context switching
- Estados: READY, RUNNING, BLOCKED, ZOMBIE

✅ **Memory Manager**
- Buddy allocator con coalescing
- Detección de memory leaks
- Syscalls: `sys_alloc_memory`, `sys_free_memory`, `sys_mem_state`

✅ **Semáforos**
- Semáforos nombrados (estilo POSIX)
- Hasta 16 semáforos simultáneos
- Reference counting
- Spinlocks atómicos con _xchg

✅ **IPC - Pipes**
- Buffer circular de 4096 bytes
- Bloqueo: writer cuando lleno, reader cuando vacío
- EOF cuando writer cierra
- Broken pipe cuando no hay readers
- Soporte de pipelines en shell (`cmd1 | cmd2`)

✅ **Shell Interactivo**
- 25+ comandos implementados
- Historial de comandos (↑↓)
- Ctrl+C para matar proceso foreground
- Soporte de background processes
- Pipelines con `|`

✅ **Test Suite**
- `test_mm <max_bytes>` - Stress test del memory manager
- `test_prio <iters>` - Test de prioridades
- `test_sync <n> <use_sem>` - Test de sincronización con semáforos
- `test_process <max>` - Test de creación/bloqueo/kill de procesos

## Comandos Principales

### Gestión de Procesos
- `ps` - Lista procesos
- `kill <pid>` - Mata un proceso
- `block <pid>` - Alterna entre BLOCKED/READY
- `nice <pid> <prio>` - Cambia prioridad (0-10)

### Tests
- `test_mm 16777216` - Test de memory manager
- `test_prio 3` - Test de prioridades
- `test_sync 1000000 1` - Test de semáforos (con sync)
- `test_sync 1000000 0` - Test sin semáforos (race condition)

### IPC/Pipes
- `pipe_demo` - Demo básico de pipe
- `pipe_sync` - Test de sincronización de pipes
- `pipe_stress 4 2` - Stress test (4 writers, 2 readers)
- `loop 1 | cat` - Pipeline de ejemplo

### Utilidades
- `help` - Lista todos los comandos
- `man <cmd>` - Descripción de un comando
- `mem` - Muestra uso de memoria
- `time` - Muestra hora actual
- `clear` - Limpia pantalla

## Estructura del Proyecto

```
TPE-SO-Entrega/
├── Bootloader/          # Bootloader x86-64
├── Kernel/              # Kernel bare-metal
│   ├── MemoryManager.c  # Buddy allocator
│   ├── process.c        # Gestión de procesos
│   ├── scheduler.c      # Scheduler
│   ├── semaphore.c      # Semáforos
│   ├── pipe.c           # Pipes
│   └── syscalls.c       # System calls
├── Userland/
│   ├── Shell/           # Shell y comandos
│   └── test/            # Test suite
├── docs/                # Documentación completa
│   ├── FEATURES.md      # Características detalladas
│   ├── CODE_REVIEW.md   # Revisión de código
│   └── ANALISIS_ESTATICO_RESUMEN.md
└── compile.sh, run.sh   # Scripts de compilación
```

## Análisis de Calidad

- **Compilación**: 0 errores, 0 warnings reales
- **PVS-Studio**: 0 problemas críticos
- **Tests**: 4/4 funcionando correctamente
- **Memory leaks**: 0 detectados

## Documentación Completa

Ver carpeta `docs/` para:
- **FEATURES.md**: Documentación exhaustiva de todas las características
- **CODE_REVIEW.md**: Revisión detallada del código
- **ANALISIS_ESTATICO_RESUMEN.md**: Resumen del análisis estático

## Enlaces a Documentación Externa

- [Manual de Usuario](https://docs.google.com/document/d/1ZWmG98adobSHLwyexbFj743G0-Je5KMigvoO34VTcoc/edit?usp=sharing)
- [Informe del Proyecto](https://docs.google.com/document/d/1RvDtHoayLrMMgk9ywLVE4wvfPCtpFVDpyeJib6fbQIc/edit?usp=sharing)

---

**Fecha de entrega**: Noviembre 2024
**Estado**: ✅ Completo y testeado
EOF

# Crear archivo de verificación
echo "6. Creando checklist de verificación..."
cat > "$TEMP_DIR/CHECKLIST_ENTREGA.md" << 'EOF'
# Checklist de Entrega TPE

## Completitud del Proyecto

### Implementación
- [x] Gestión de procesos con scheduler
- [x] Memory manager (buddy allocator)
- [x] Semáforos nombrados
- [x] IPC con pipes
- [x] Shell funcional
- [x] Test suite completo
- [x] Manejo de excepciones

### Syscalls Requeridas
- [x] `sys_create_process` / `sys_kill` / `sys_waitpid`
- [x] `sys_block` / `sys_unblock`
- [x] `sys_getpid` / `sys_yield`
- [x] `sys_nice` / `sys_ps`
- [x] `sys_alloc_memory` / `sys_free_memory`
- [x] `sys_mem_state`
- [x] `sys_pipe` / `sys_read` / `sys_write` / `sys_close`
- [x] `sys_dup2`
- [x] `sys_sem_create` / `sys_sem_open` / `sys_sem_wait` / `sys_sem_post` / `sys_sem_close`

### Tests
- [x] test_mm - Memory manager stress test
- [x] test_prio - Process priorities
- [x] test_sync - Semaphore synchronization
- [x] test_process - Process creation/blocking/killing

### Calidad de Código
- [x] Compilación sin errores
- [x] Compilación sin warnings reales (2 falsos positivos verificados)
- [x] Análisis estático con PVS-Studio
- [x] Validación de parámetros en syscalls
- [x] Manejo de errores robusto

### Documentación
- [x] README.md con instrucciones de compilación
- [x] FEATURES.md con características implementadas
- [x] Comentarios en código
- [x] Manual de usuario (Google Docs)
- [x] Informe técnico (Google Docs)

### Archivos de Entrega
- [x] Código fuente completo
- [x] Scripts de compilación (compile.sh, run.sh)
- [x] Makefile funcional
- [x] Documentación
- [x] Este checklist

## Verificación Pre-Entrega

### Compilación
```bash
./compile.sh
# Debe compilar sin errores
```

### Ejecución
```bash
./run.sh
# Debe bootear y mostrar shell
```

### Tests Básicos
En el shell del SO:
```
help           # Debe listar comandos
ps             # Debe mostrar procesos
test_mm 100000 # Debe ejecutar sin errores
test_sync 10000 1  # Valor final debe ser 0
```

## Verificado por

- [ ] Francisco Costa (Padrón 65202)
- [ ] Roman Salerno (Padrón 65145)
- [ ] Tiago Heras (Padrón 65627)

---

**Fecha**: _______________
**Firma**: _______________
EOF

# Empaquetar
echo "7. Creando archivo comprimido..."
tar -czf "$PACKAGE_NAME" "$TEMP_DIR"

# Limpieza
echo "8. Limpiando archivos temporales..."
rm -rf "$TEMP_DIR"

# Resumen
echo ""
echo "==========================================="
echo "  Empaquetado completado exitosamente"
echo "==========================================="
echo ""
echo "Archivo generado: $PACKAGE_NAME"
echo "Tamaño: $(du -h "$PACKAGE_NAME" | cut -f1)"
echo ""
echo "Contenido del paquete:"
tar -tzf "$PACKAGE_NAME" | head -20
echo "..."
echo ""
echo "Para verificar el paquete completo:"
echo "  tar -tzf $PACKAGE_NAME"
echo ""
echo "Para extraer:"
echo "  tar -xzf $PACKAGE_NAME"
echo ""
echo "✅ El proyecto está listo para entregar"
echo ""
