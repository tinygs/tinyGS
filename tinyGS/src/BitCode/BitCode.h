/*
  BitCode.h - BitCode class

  Copyright (C) 2022 -2023 @estbhan

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef BITCODE_H
#define BITCODE_H
#include <stdint.h> //uint8_t

class BitCode{
    public:

static uint8_t read_bit_from_byte(uint8_t byte, int posicion_bit);
static int nrz2nrzi (uint8_t *entrada, size_t sizeEntrada, uint8_t *salida, size_t *sizeSalida);
static void write_bit_on_byte(uint8_t *byte, int k, int dato);
static int remove_bit_stuffing (uint8_t *entrada, size_t sizeEntrada, uint8_t *salidabin, size_t *sizeSalida);
static void invierte_bits_de_un_byte(uint8_t br, uint8_t *bs);
static void invierte_bytes_de_un_array(uint8_t *entrada, size_t sizeEntrada, uint8_t *salida, size_t *sizeSalida);
static int nrz2ax25(uint8_t *entrada, size_t sizeEntrada,  uint8_t *ax25bin, size_t *sizeAx25bin);
static int pn9(uint8_t *entrada, size_t sizeEntrada, uint8_t *salida);
};
#endif
