/*
 *  engrave --- preparation of image files for adaptive screening
 *              in a conventional RIP (Raster Image Processor).
 *
 *  Copyright (C) 2018 Yuri V. Kouznetsov, Paul A. Wolneykien.
 *
 *  This program is free software: you can redistribute it and/or
 *  modify it under the terms of the GNU Affero General Public License
 *  as published by the Free Software Foundation, either version 3 of
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public
 *  License along with this program.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 *  Contact information:
 *
 *  The High Definition Screening project:
 *    <https://github.com/wolneykien/engrave/>.
 *
 *  Yuri Kuznetsov <yurivk@mail.ru).
 *
 *  Paul Wolneykien <manowar@altlinux.org).
 *
 *  The Graphic Arts Department of the
 *  North-West Institute of Printing of the
 *  Saint-Petersburg State University of Technology and Design
 *
 *  191180 Saint-Petersburg Jambula lane 13.
 *
 */

/* Библиотека весовых функций. */

#include <string.h>
#include <stdio.h>
#include "weightfunc.h"

/**
 * Весовая функция вертикальной линии.
 */
const unsigned char ortline[WEIGHTFUNC_LEN] = {
    0xdc, 0x87, 0x32, 0x07, 0x5c, 0xb2,

    0xf2, 0x9c, 0x47, 0x1c, 0x72, 0xc7,

    0xe4, 0x8e, 0x39, 0x0e, 0x64, 0xb9,

    0xf9, 0xa4, 0x4e, 0x24, 0x79, 0xce,

    0xeb, 0x95, 0x40, 0x15, 0x6b, 0xc0,

    0xff, 0xab, 0x55, 0x2b, 0x80, 0xd5
};

/**
 * Весовая функция диагональной линии (NW--SE).
 */
const unsigned char dialine[WEIGHTFUNC_LEN] = {
    0x07, 0x55, 0x95, 0xc7, 0xeb, 0xff,

    0x32, 0x1c, 0x6b, 0xa4, 0xd5, 0xf2,

    0x79, 0x47, 0x0e, 0x5c, 0x9c, 0xce,

    0xb2, 0x87, 0x39, 0x24, 0x72, 0xab,

    0xdc, 0xc0, 0x80, 0x4e, 0x15, 0x64,

    0xf9, 0xe4, 0xb9, 0x8e, 0x40, 0x2b
};

/**
 * Вертикальный контур (W).
 */
const unsigned char ortcontour[WEIGHTFUNC_LEN] = {
    0x07, 0x32, 0x5c, 0x87, 0xb2, 0xdc,

    0x1c, 0x47, 0x72, 0x9c, 0xc7, 0xf2,

    0x0e, 0x39, 0x64, 0x8e, 0xb9, 0xe4,

    0x24, 0x4e, 0x79, 0xa4, 0xce, 0xf9,

    0x15, 0x40, 0x6b, 0x95, 0xc0, 0xeb,

    0x2b, 0x55, 0x80, 0xab, 0xd5, 0xff
};

/**
 * Диагональный контур (SW).
 */
const unsigned char diacontour[WEIGHTFUNC_LEN] = {
    0x72, 0x9c, 0xc0, 0xdc, 0xf2, 0xff,

    0x4e, 0x87, 0xb2, 0xce, 0xeb, 0xf9,

    0x32, 0x64, 0x79, 0xa4, 0xc7, 0xe4,

    0x1c, 0x40, 0x55, 0x8e, 0xb9, 0xd5,

    0x0e, 0x2b, 0x39, 0x6b, 0x80, 0xab,

    0x07, 0x15, 0x24, 0x47, 0x5c, 0x95
};

/**
 * Диагональный угол (NW).
 */
const unsigned char ortcorner[WEIGHTFUNC_LEN] = {
    0x07, 0x0e, 0x24, 0x47, 0x79, 0xb9,

    0x15, 0x1c, 0x32, 0x64, 0xa4, 0xe4,

    0x2b, 0x39, 0x40, 0x55, 0x87, 0xc7,

    0x4e, 0x6b, 0x5c, 0x72, 0xb2, 0xf2,

    0x80, 0x9c, 0x8e, 0xab, 0x95, 0xd5,

    0xc0, 0xeb, 0xce, 0xf9, 0xdc, 0xff
};

/**
 * Боковой угол (W).
 */
const unsigned char diacorner[WEIGHTFUNC_LEN] = {
    0x32, 0x64, 0x8e, 0xb9, 0xdc, 0xf9,

    0x1c, 0x47, 0x72, 0x9c, 0xc7, 0xeb,

    0x07, 0x15, 0x55, 0x80, 0xab, 0xd5,

    0x0e, 0x2b, 0x39, 0x5c, 0x87, 0xb2,

    0x24, 0x4e, 0x79, 0xa4, 0xce, 0xf2,

    0x40, 0x6b, 0x95, 0xc0, 0xe4, 0xff
};

/**
 * Библиотека весовых функций тайлов.
 */
unsigned char weightfuncs[WEIGHTFUNCS_COUNT][WEIGHTFUNC_LEN];


static void copyweightfunc( unsigned char *dest,
							const unsigned char *src,
							int rotation );

/**
 * Инициализирует библиотеку весовых функций.
 */
void
weightfuncs_init ()
{
	static int initialized = 0;

	if ( initialized ) return;
	
	copyweightfunc( weightfuncs[TILE_NL - 1],  ortline,     0   );
	copyweightfunc( weightfuncs[TILE_WL - 1],  ortline,    -90  );
	copyweightfunc( weightfuncs[TILE_NWL - 1], dialine,     0   );
	copyweightfunc( weightfuncs[TILE_NEL - 1], dialine,    -90  );
	copyweightfunc( weightfuncs[TILE_WS - 1],  ortcontour,  0   );
	copyweightfunc( weightfuncs[TILE_NS - 1],  ortcontour, -90  );
	copyweightfunc( weightfuncs[TILE_ES - 1],  ortcontour,  180 );
	copyweightfunc( weightfuncs[TILE_SS - 1],  ortcontour,  90  );
	copyweightfunc( weightfuncs[TILE_NES - 1], diacontour,  180 );
	copyweightfunc( weightfuncs[TILE_SES - 1], diacontour,  90  );
	copyweightfunc( weightfuncs[TILE_SWS - 1], diacontour,  0   );
	copyweightfunc( weightfuncs[TILE_NWS - 1], diacontour, -90  );
	copyweightfunc( weightfuncs[TILE_WC - 1],  diacorner,   0   );
	copyweightfunc( weightfuncs[TILE_NC - 1],  diacorner,  -90  );
	copyweightfunc( weightfuncs[TILE_EC - 1],  diacorner,   180 );
	copyweightfunc( weightfuncs[TILE_SC - 1],  diacorner,   90  );
	copyweightfunc( weightfuncs[TILE_NWC - 1], ortcorner,   0   );
	copyweightfunc( weightfuncs[TILE_NEC - 1], ortcorner,  -90  );
	copyweightfunc( weightfuncs[TILE_SEC - 1], ortcorner,  180  );
	copyweightfunc( weightfuncs[TILE_SWC - 1], ortcorner,   90  );

	initialized = 1;
}

/**
 * Возвращает массив весовых значений по номеру тайла.
 */
const unsigned char *
get_weight_func( unsigned char tile_index )
{
	if ( tile_index <= WEIGHTFUNCS_COUNT ) {
		return weightfuncs[tile_index - 1];
	}
}

/**
 * Делает срез весовой функции #tile_index по уровню тона #tile_area
 * и помещает результат в #tilebuf.
 */
void weight_func_apply( unsigned char *tilebuf,
						unsigned char tile_index,
						unsigned char tile_area )
{
	if ( 0 == tile_area || TILE_EMPTY == tile_index ) {
		memset( tilebuf, 0, WEIGHTFUNC_LEN );
	} else {
		const unsigned char *weight_func = get_weight_func( tile_index );
		if ( !weight_func ) {
			fprintf( stderr, "Error: Unable to get the weight function "
					 "of tile %d\n", tile_index );
			return;
		}

		int x;
		for ( x = 0; x < WEIGHTFUNC_LEN; x++ ) {
			if ( weight_func[x] <= tile_area ) {
				tilebuf[x] = 0xff;
			} else {
				tilebuf[x] = 0x00;
			}
		}
	}
}

static void
copyweightfunc( unsigned char *dest, const unsigned char *src,
				int rotation )
{
	int i, i0, j, j0;

	for (j = 0; j < TILEHEIGHT; j++) {
		for (i = 0; i < TILEWIDTH; i++) {
			switch ( rotation ) {
			case 0:
				j0 = j;
				i0 = i;
				break;
			case 90:
				j0 = i;
				i0 = TILEWIDTH - j - 1;
				break;
			case 180:
			case -180:
				j0 = TILEHEIGHT - j - 1;
				i0 = TILEWIDTH - i  - 1;
				break;
			case -90:
				j0 = TILEHEIGHT - i - 1;
				i0 = j;
				break;
			default:
				fprintf( stderr, "BUG: Illegal rotation: %i\n",
						 rotation );
				return;
			}
			dest[ j * TILEWIDTH + i ] = src[ j0 * TILEWIDTH + i0 ];
		}
	}
}
