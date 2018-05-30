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


#ifndef __FILTER_H
#define __FILTER_H

/* Типы и константы, описание интерфейсов функций, которые используются при
 * реализации фильтров.
 */

#include <getopt.h>

/* Имя инициированной комманды. */
extern char *program_name;

/* Номер программного процеса. */
extern pid_t pid;

/* Порядковый номер фильтра. */
extern int fidx;

/* Признак режима повышенной информативности. */
extern int want_verbose;

/* Парметры фильтрации */
/* Параметры изображения: */
extern unsigned long int width;		/* ширина изображения; */
extern int unsigned long height;        /* высота изображения; */
extern float hres;                      /* разрешение по горизонтали; */
extern float vres;			/* разрешение по вертикали; */
extern int is_cmyk;			/* признак 4-красочного изображения; */
extern int miniswhite;			/* признак негативного изображения. */

typedef enum { FILTER_EPS_FMT, FILTER_TIFF_FMT } filter_outformat_t;
extern filter_outformat_t filter_outformat;

/* Тип функции, печатающей заголовок краткой справки. */
typedef void(*usage_header_f)(FILE *out);

/* Тип функции, печатающей краткую справку по специальным параметрам
 *  фильтра.
 */
typedef void(*usage_params_f)(FILE *out);

const char *get_tmp_filter_file_name( const char *fsuf, int color_idx );

/**
 * Структура с функциями кодировщика фильтра.
 */
struct filter_writer {
	/**
	 * Инициализирует кодировщик для записи карты тайлов
	 * в файл с именем #outfile. Принимает признак #mask негативного
	 * изображения (маски). Возвращает указатель на контекст,
	 * который затем используется в других функциях.
	 */
	void * (*open_tilemap)      ( const char *outfile, int mask );
	
	/**
	 * Инициализирует кодировщик для записи тонового изображения
	 * в файл с именем #outfile. Возвращает указатель на контекст,
	 * который затем используется в других функциях.
	 */
	void * (*open_tonemap)      ( const char *outfile );

	/**
	 * Кодирует #zl строк тайлов. Если #zl > 1, то дополнительно
     * записываются пустые строки.
	 */
	void   (*write_tile_lines) ( void *ctx, unsigned int zl );

	/**
	 * Кодирует #z пробелов.
	 */
	void   (*write_spaces)      ( void *ctx, unsigned int z );

	/**
	 * Кодирует тайл с номером #tile_index и относительной
	 * площадью #tile_area.
	 */
	void   (*write_tile)        ( void *ctx,
								  unsigned char tile_index,
								  unsigned char tile_area );

	/**
	 * Выводит строку тонового изображения. Строка из #count отсчётов
	 * по #ss байт передаётся в буфере #buf.
	 */
	void  (*write_toneline)     ( void *ctx, const char *buf,
								  size_t ss, size_t count );

	/**
	 * Закрывает кодировщик и освобождает ресурсы.
	 */
	void   (*close)             ( void *ctx );
};

char *get_filter_option(int i, char *dst_fopt, char *fopts);

int decode_switches (int argc, char **argv, int error_code, \
		     struct option const *long_options, \
		     char ***option_vars, usage_header_f usage_header, \
		     usage_params_f usage_params);

void write_outbuf(char *outbuf, size_t ss, size_t len);

/**
 * Возвращает указатель на выбранный кодировщик.
 * Должна вызываться после разбора опций.
 */
struct filter_writer *get_selected_filter_writer ();

#endif /* __FILTER_H */
