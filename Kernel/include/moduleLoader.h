#ifndef MODULELOADER_H
#define MODULELOADER_H

/**
 * @brief Copia los módulos de kernel desde el payload a sus direcciones finales.
 *
 * @param payloadStart Dirección donde se encuentran empaquetados los módulos.
 * @param moduleTargetAddress Arreglo con los destinos donde debe copiarse cada módulo.
 */
void loadModules(void * payloadStart, void ** moduleTargetAddress);

#endif
