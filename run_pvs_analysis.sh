#!/bin/bash
set -e

echo "=========================================="
echo "  Análisis estático con PVS-Studio"
echo "=========================================="

# Limpiar análisis previos
rm -f pvs-studio.log strace_out.txt pvs-report.txt
rm -rf pvs-report

# Configurar credenciales gratuitas
echo "Configurando licencia gratuita..."
pvs-studio-analyzer credentials PVS-Studio Free FREE-FREE-FREE-FREE

# Limpiar compilación previa
echo "Limpiando compilación anterior..."
make clean > /dev/null 2>&1

# Generar compile_commands.json usando strace
echo "Generando información de compilación..."
pvs-studio-analyzer trace -- make all 2>&1 | tee build.log

# Ejecutar análisis
echo "Ejecutando análisis estático de PVS-Studio..."
pvs-studio-analyzer analyze -o pvs-studio.log -j4 \
    -e /usr/include \
    -e Bootloader \
    -e Toolchain \
    -e Image

# Convertir a formato legible
echo "Generando reporte..."
plog-converter -a GA:1,2 -t fullhtml -o pvs-report pvs-studio.log
plog-converter -a GA:1,2 -t tasklist -o pvs-report.txt pvs-studio.log

echo ""
echo "=========================================="
echo "  Análisis completado"
echo "=========================================="
echo ""
echo "Reportes generados:"
echo "  - HTML: pvs-report/index.html"
echo "  - Texto: pvs-report.txt"
echo ""
echo "Para ver el reporte HTML, abrí: pvs-report/index.html"
echo "Para ver errores críticos: grep 'High\|Medium' pvs-report.txt"
echo ""

# Mostrar resumen
if [ -f pvs-report.txt ]; then
    echo "Resumen de warnings encontrados:"
    grep -E "High|Medium|Low" pvs-report.txt | wc -l
    echo ""
    echo "Top 10 warnings más importantes:"
    head -20 pvs-report.txt
fi
