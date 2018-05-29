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


/* Библиотека вспомогательных функций. */

/* Определяет предельную длину строки символов, если таковая ещё не
 * определена. */
#ifndef MAXLINE
#define MAXLINE 4096
#endif

/* Комманды языка PostScript для установки цветовых пространств, соответствующих
 * индивидуальным красителям.
 */
#define CYAN_COLOR "[ /Separation (Cyan) /DeviceCMYK { 0 0 0 } ] setcolorspace"
#define MAGENTA_COLOR "[ /Separation (Magenta) /DeviceCMYK { 0 0 0 4 1 roll } ] setcolorspace"
#define YELLOW_COLOR "[ /Separation (Yellow) /DeviceCMYK { 0 0 0 4 2 roll } ] setcolorspace"
#define BLACK_COLOR "[ /Separation (Black) /DeviceCMYK { 0 0 0 4 3 roll } ] setcolorspace"

/* Суффиксы красителей */
#define CYAN_SUF "c"
#define MAGENTA_SUF "m"
#define YELLOW_SUF "y"
#define BLACK_SUF "k"

/* Наименования красителей. */
#define	CYAN_COLOR_NAME "Cyan"
#define	MAGENTA_COLOR_NAME "Magenta"
#define	YELLOW_COLOR_NAME "Yellow"
#define	BLACK_COLOR_NAME "Black"

/* Операции с именами файлов. */

void pathcat(char *dest, const char *src);
char *extract_file_name(char *full_name);
void delete_ext(char *filename);

/* Тип функции, используемой для аварифной очистки занятых ресурсов. */
typedef void(*cleanup_f)();

/* Функции управления аварийной очисткой занятых ресурсов. */

void init_cleanup();
int push_cleanup(cleanup_f cleanup);
cleanup_f pop_cleanup();

const char *get_tmp_file_name( const char *fsuf, pid_t pid, int fidx,
							   int color_idx );

/* Функции для работы с цветовыми каналами. */
char *get_cmyk_color(int i);
char *get_cmyk_color_suf(int i);
char *get_cmykcolor_name(int i);
void set_cyan_index(int i);
void set_magenta_index(int i);
void set_yellow_index(int i);
void set_black_index(int i);

/* Функции для работы с данными изображения. */
void invertsmp(void *buf, size_t ss, size_t count);
size_t freadsmp(void *buf, size_t ss, size_t count, FILE *stream, int neg); 
size_t fwritesmp(void *buf, size_t ss, size_t count, FILE *stream, int neg, void *outbuf);


