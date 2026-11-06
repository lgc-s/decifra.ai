/* Salvamento em disco: arquivo binário versionado com checksum. */
#ifndef SAVE_H
#define SAVE_H

#include <stdbool.h>
#include "systems/progression.h"

#define SAVE_FILE "revia_save.dat"

/* Escreve em arquivo temporário e renomeia, para nunca corromper o save anterior. */
bool save_write(const Progress *p, const char *path);

/* Lê e valida (assinatura, versão, checksum e limites dos campos). */
bool save_read(Progress *p, const char *path);

bool save_exists(const char *path);

#endif
