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


#ifndef __WEIGHTFUNC_H
#define __WEIGHTFUNC_H


/* Библиотека весовых функций. */

#define TILE_EMPTY 0  /// The empty tile
#define TILE_NL    1  /// Vertical line
#define TILE_WL    2  /// Horizontal line
#define TILE_NWL   3  /// North-West diagonal line
#define TILE_NEL   4  /// North-East diagonal line
#define TILE_WS    5  /// West contour side
#define TILE_NS    6  /// North contour side
#define TILE_ES    7  /// East contour side
#define TILE_SS    8  /// South contour side
#define TILE_NES   9  /// North-East contour side
#define TILE_SES   10 /// South-East contour side
#define TILE_SWS   11 /// South-West contour side
#define TILE_NWS   12 /// North-West contour side
#define TILE_WC    13 /// West corner
#define TILE_NC    14 /// North corner
#define TILE_EC    15 /// East corner
#define TILE_SC    16 /// South couner
#define TILE_NWC   17 /// North-West couner
#define TILE_NEC   18 /// North-East corner
#define TILE_SEC   19 /// South-East corner
#define TILE_SWC   20 /// South-West corner

/**
 * Количество весовых функций.
 */
#define WEIGHTFUNCS_COUNT 20

/**
 * Ширина тайла в пикселях.
 */
#define TILEWIDTH 6

/**
 * Высота тайла в пикселях.
 */
#define TILEHEIGHT 6

/**
 * Длина весовой функции в байтах.
 */
#define WEIGHTFUNC_LEN TILEWIDTH * TILEHEIGHT


/**
 * Инициализирует библиотеку весовых функций.
 */
void weightfuncs_init ();

/**
 * Возвращает массив весовых значений по номеру тайла.
 */
const unsigned char *get_weight_func( unsigned char tile_index );

/**
 * Делает срез весовой функции #tile_index по уровню тона #tile_area
 * и помещает результат в #tilebuf.
 */
void weight_func_apply( unsigned char *tilebuf,
						unsigned char tile_index,
						unsigned char tile_area );

#endif /* __WEIGHTFUNC_H */
