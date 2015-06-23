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


/*
 * Фильтр bg генерирует PostScript-код для отрисовки однородного фона.
 * Предназначен, главным образом, для тестирования работы других фильтров.
*/

#include <stdio.h>
#include <sys/types.h>
#include "system.h"
#include "filter.h"
#include "misc.h"

/* Код возврата, означающий ошибочное завершение программы. */
#define EXIT_FAILURE 1

/* Коды возвраты для функции разбора параметров с длинными именами. */
enum {DUMMY_CODE=129
};

/* Переменная для хранения значения тона для фона. */
char *bg_value;

/* Определение параметров командной строки. */
struct option const long_options[] =
{
  {"value", required_argument, NULL, 0},
  {NULL, 0, NULL, 0}
};

/* Указатели на переменные, соответствующие параметрам коммандной строки. */
char **option_vars[] =
{
	&bg_value
};

/* Вывести в указанный поток заголовок фрагмента PostScript-программы. */
void write_header(FILE *stream) {

	fprintf(stream, "%% Filter: bg filter from "PACKAGE" "VERSION"\n"
			"%%%%LanguageLevel 2\n"
			"gsave\t%% Save graphics state\n"
			"%s setcolor\n"
			"%f %f scale\n"
		        "0 0 %u %u rectfill\n",
		        bg_value,
			(float) width/hres*72,
			(float) height/vres*72,
			width, height);

}

/* Вывести в указанный поток окончание фрагмента PostScript-программы. */
void write_footer(FILE *stream) {

	fprintf(stream, "grestore\t%% Restore previous graphic state\n");

}

/* Вывод заголовка краткой справки. */
void
usage_header(FILE *out)
{
  printf (_("%s - \
Image filter for 'engrave' package. Produces PostScript code for drawing a simple background, reading a RAW CT image data from stdin for convenient.\n"), program_name);

}

/* Вывод краткой справки по специальным праметрам. */
void
usage_params(FILE *out)
{

  fprintf(out, _("\
  --value=NUMBER		background tone value\n\
"));

}

/* Основная функция. */
int
main (int argc, char **argv)
{

  /* Хранит номер первого необработанного аргумента. */
  int opt_r;

  /* Размер отсчёта в байтах. */
  size_t ss;
  
  /* Масив файловых указателей для оновременной записи в 4 временных
   * файла.
   */
  FILE *outfile[] = {
	  NULL,
	  NULL,
	  NULL,
	  NULL
  };
  
  /* Набор из 4 строк для хранения имён временных файлов. */
  char filenames[4][MAXLINE] = { "", "", "", "" };
  
  /* Буфер для хранения одной строки изображения. */
  char *buf = NULL;

  /* Счётчик цвеовых каналов. */
  int c0, c, cN;

  /* Контрольный счётчик ввода-вывода. */
  size_t rd;

  /* Номер строки изображения. */
  int y;

  /* Признак успешного завершения работы фильтра. */
  int OK = 0;

  /* Функция аварийной очистки занятых ресурсов. */
  void cleanup() {
	  int i;

	  if (!OK)
		  fprintf(stderr, "%s: Finished with error.\n", program_name);
	  for (i = 0; i < 4; i++) {
		  if (outfile[i] != NULL)
			  fclose(outfile[i]);
		  if (!OK && filenames[i][0] != '\0') {
			  fprintf(stderr, "%s: Delete temporary file: %s\n", program_name, filenames[i]);
			  unlink(filenames[i]);
		  }
	  }
	  if (buf != NULL)
		  free(buf);
  }

  /* Начало работы фильтра. Обработка аргументов коммандной строки */

  /* Получение имени комманды. */
  program_name = argv[0];

  /* Инициализация функции аварийной очистки. */
  init_cleanup(program_name);
  push_cleanup(cleanup);

  /* Разбор аргуентов командной строки. */
  opt_r = decode_switches (argc, argv, EXIT_FAILURE, long_options, option_vars, &usage_header, &usage_params);

  /* Проверка наличия параметров изображения. */
  if (!width || !height || !hres || !vres) {
	  fprintf(stderr, "%s: WIDTH, HEIGHT, HRES & VRES should be specified.\n", program_name);
	  /* Выход с признаком ошибки, если были переданы не все параметры. */
	  exit(EXIT_FAILURE);
  }
  
  /* Инициализация счётчика цветовых каналов и размера отсчёта в соответствие с
   * цветовой схемой изображения.
   */
  if (is_cmyk) {
	  ss = 4;	/* 4 байта для CMYK */
	  c0 = 0;	/* 4 цвета, с 0 */
	  cN = 3;	/* по 3 */
  } else {
	  ss = 1;	/* 1 байт для Grayscale */
	  c0 = 3;	/* 1 цвет с 0 */
	  cN = 3;	/* по 0 */
  }

  /* Выделение буфера для хранения строки изображения. */
  buf = calloc(ss, width);
  if (buf == NULL) {
	  fprintf(stderr, "%s: Scanline buffer allocation failed\n", program_name);
	  /* Выход с признаком ошибки, если буфер выделить не удалось. */
	  exit(EXIT_FAILURE);
  }

  /* Создание временных PostScript-файлов. */
  for (c = c0; c <= cN; c++) {
	 /* Открытие файла с именем, соответствующим номеру цветового канала. */
	 if ((outfile[c] = open_tmp_file("ct", c, filenames[c])) == NULL) {
		/* Выход с признаком ошибки, если файл не был открыт. */
		exit(EXIT_FAILURE);
	 }
	 /* Запись заголовка. */
	 write_header(outfile[c]);
  }

  /* Последованиельное чтение строк изображения со стандартного входа.
   */
  for (y = 0; y < height; y++) {
	  /* Чтение строки изображения. */
	  rd = fread(buf, ss, width, stdin);
	  /* Контроль количества прочитанных отсчётов. */
	  if (rd < width) {
	    break;
	  }
  }

  /* Запись завершающей части PostScript-кода. */
  for (c = c0; c <= cN; c++) {
	  write_footer(outfile[c]);
  }

  /* Установка признака успешного завершения. */
  OK = 1;
  
  /* Освобождение занятых ресурсов. */
  (*pop_cleanup())();

  /* Выход с признаком успешного завершения работы. */
  exit (0);

}

