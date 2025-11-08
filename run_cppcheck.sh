#!/bin/bash
set -e

echo "=========================================="
echo "  Análisis estático con cppcheck"
echo "=========================================="

# Instalar cppcheck si no está instalado
if ! command -v cppcheck &> /dev/null; then
    echo "Instalando cppcheck..."
    sudo apt-get update && sudo apt-get install -y cppcheck
fi

echo "Ejecutando análisis con cppcheck..."

# Ejecutar cppcheck en el kernel
cppcheck --enable=all \
    --suppress=missingIncludeSystem \
    --suppress=unusedFunction \
    --suppress=unmatchedSuppression \
    -I Kernel/include \
    -I Kernel/font_assets \
    --platform=unix64 \
    --std=c99 \
    --force \
    --quiet \
    --template='{file}:{line}: {severity}: {message} [{id}]' \
    Kernel/ \
    2> cppcheck-kernel.txt

# Ejecutar cppcheck en userland
cppcheck --enable=all \
    --suppress=missingIncludeSystem \
    --suppress=unusedFunction \
    --suppress=unmatchedSuppression \
    -I Userland/include \
    -I Userland/include/libc \
    -I Userland/include/libsys \
    -I Userland/test/include \
    -I Kernel/include \
    --platform=unix64 \
    --std=c99 \
    --force \
    --quiet \
    --template='{file}:{line}: {severity}: {message} [{id}]' \
    Userland/ \
    2> cppcheck-userland.txt

# Combinar reportes
cat cppcheck-kernel.txt cppcheck-userland.txt > cppcheck-full.txt

echo ""
echo "=========================================="
echo "  Análisis completado"
echo "=========================================="
echo ""
echo "Reportes generados:"
echo "  - Kernel: cppcheck-kernel.txt"
echo "  - Userland: cppcheck-userland.txt"
echo "  - Completo: cppcheck-full.txt"
echo ""

# Mostrar resumen
TOTAL=$(cat cppcheck-full.txt | wc -l)
echo "Total de warnings encontrados: $TOTAL"
echo ""

if [ $TOTAL -gt 0 ]; then
    echo "Top 15 problemas más importantes:"
    cat cppcheck-full.txt | grep -E "error:|warning:" | head -15
fi
