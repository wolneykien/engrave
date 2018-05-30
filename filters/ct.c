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


/*
 * Фильтр ct генерирует PostScript-код для представления каждого канала
 * цифрового изображения, получаемого со стандартного входа. Данный фильтр не
 * изменяет изображение.
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

/* Данный фильтр не имеет специальных параметров. */
struct option const long_options[] =
{
	{NULL, 0, NULL, 0}
};

/* Вывод заголовка краткой справки. */
void
usage_header(FILE *out)
{
  printf (_("%s - \
Image filter for 'engrave' package. Produces PostScript code for each image color channel reading a RAW CT image data from stdin\n"), program_name);

}

/* Основная функция. */
int
main (int argc, char **argv)
{

  /* Хранит номер первого необработанного аргумента. */
  int opt_r;

  /* Размер отсчёта в байтах. */
  size_t ss;
  
  /* Набор из 4 строк для хранения имён временных файлов. */
  const char *filenames[4] = { NULL, NULL, NULL, NULL };

  /* Набор функций кодировщика тайлов. */
  struct filter_writer *filter_writer_p = get_selected_filter_writer();
  
  /* Набор из 4 структур для хранения информации при кодировании цифровых
   * изображений в символьное представление.
   */
  void *filter_writer_ctx[] = {
	  NULL,
	  NULL,
	  NULL,
	  NULL
  };

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
		  if (filter_writer_ctx[i] != NULL) {
			  filter_writer_p->close( filter_writer_ctx[i] );
			  filter_writer_ctx[i] = NULL;
		  }
		  if (!OK && filenames[i] != NULL) {
			  fprintf( stderr, "%s: Delete temporary file: %s\n", program_name,
					  filenames[i] );
			  unlink( filenames[i] );
			  free( filenames[i] );
			  filenames[i] = NULL;
		  }
	  }
	  if (buf != NULL)
		  free(buf);
  }

  /* Начало работы фильтра. Обраьотка аргументов коммандной строки */

  /* Получение имени комманды. */
  program_name = argv[0];

  /* Инициализация функции аварийной очистки. */
  init_cleanup(program_name);
  push_cleanup(cleanup);

  /* Разбор аргуметов командной строки. */
  opt_r = decode_switches (argc, argv, EXIT_FAILURE, long_options, NULL, &usage_header, NULL);

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

  /* Создание временных PostScript-файлов и соответствующих структур
   * для кодирования избражения в символьное представление.
   */
  for (c = c0; c <= cN; c++) {
	 /* Получение имени временного файла. */
	 if ((filenames[c] = get_tmp_filter_file_name( "ct", c )) == NULL) {
		/* Выход с признаком ошибки, если имя не получено. */
		fprintf( stderr, "%s: Can't get temp file name\n", program_name );
		exit(EXIT_FAILURE);
	 }
	 /* Создание и инициализация информации для кодирования. */
	 filter_writer_ctx[c] = filter_writer_p->open_tonemap( filenames[c] );
	 if ( filter_writer_ctx[c] == NULL ) {
		 fprintf(stderr,
				 "%s: Failed to initialize the writer.\n",
				 program_name);
		 /* Завершение программы с признаком ошибки, в случае неудачной
		  * инициализации структуры.
		  */
		 exit(EXIT_FAILURE);
	 }
  }

  /* Последованиельное чтение строк изображения со стандартного входа,
   * кодирование в символьное представление отдельных значений по каждому
   * каналу и запись закодированной информации в PostScript-файлы.
   */
  for (y = 0; y < height; y++) {
	  /* Чтение строки изображения. */
	  rd = fread(buf, ss, width, stdin);
	  /* Контроль количества прочитанных отсчётов. */
	  if (rd < width) {
		  fprintf(stderr, "%s: Line %i. Image stream suddenly closed\n", program_name, y);
		  /* Выход с признаком ошибки, если было прочитано меньше
		   * отсчётов, чем ширина изображения.
		   */
		  exit(EXIT_FAILURE);
	  }

	  /* Кодирование цветовых каналов. */
	  for (c = c0; c <= cN; c++)
		  /* Запись строки в символьном представлении. При записи
		   * последней строки устанавливается признак сброса буфера
		   * для того, чтобы можно было затем записать завершающую
		   * часть PostScript-кода. 
		   */
		  filter_writer_p->write_toneline( filter_writer_ctx[c],
										   buf + c - c0, ss,
										   width );
  }

  /* Запись завершающей части PostScript-кода. */
  for (c = c0; c <= cN; c++) {
	  filter_writer_p->close( filter_writer_ctx[c] );
	  filter_writer_ctx[c] = NULL;
  }

  /* Установка признака успешного завершения. */
  OK = 1;
  
  /* Освобождение занятых ресурсов. */
  (*pop_cleanup())();

  /* Выход с признаком успешного завершения работы. */
  exit (0);

}

