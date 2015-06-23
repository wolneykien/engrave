/*
 *  engrave --- preparation of image files for adaptive screening
 *              in a conventional RIP (Raster Image Processor).
 *
 *  Copyright (C) 2015 Yuri V. Kouznetsov, Paul A. Wolneykien.
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

/* Структура для хранения информации при кодировании цифрового изображения в
 * смвольное представление.
 */
struct ascii85 {
	int line_break;
	int offset;
	char buffer[4];
	char tuple[6];
	FILE *file;
};

/* Ширина строки символов закодированного изображения. */
#define LINEWIDTH 75

/* Тип функции, печатающей заголовок краткой справки. */
typedef void(*usage_header_f)(FILE *out);

/* Тип функции, печатающей краткую справку по специальным параметрам
 *  фильтра.
 */
typedef void(*usage_params_f)(FILE *out);

FILE *open_tmp_file(const char *fsuf, int color_idx, char *filename);

struct ascii85 *new_ascii85(FILE *outfile);
void destroy_ascii85(struct ascii85 *ascii85_p);
void ascii85_encode(struct ascii85 *a, const unsigned char code);

char *get_filter_option(int i, char *dst_fopt, char *fopts);

int decode_switches (int argc, char **argv, int error_code, \
		     struct option const *long_options, \
		     char ***option_vars, usage_header_f usage_header, \
		     usage_params_f usage_params);

void write_outbuf(char *outbuf, size_t ss, size_t len);
