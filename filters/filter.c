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


/* Библиотека функций, используемых при реализации фильтров. */

#include <stdio.h>
#include <sys/types.h>
#include "system.h"
#include "filter.h"
#include "misc.h"

/* Имя инициированной комманды. */
char *program_name;

/* Номер программного процеса. */
pid_t pid;

/* Порядковый номер фильтра. */
int fidx;

/* Признак режима повышенной информативности. */
int want_verbose;

/* Парметры фильтрации */

/* Параметры изображения: */
unsigned long int width;	/* ширина изображения; */
int unsigned long height;	/* высота изображения; */
float hres;             	/* разрешение по горизонтали; */
float vres;			/* разрешение по вертикали; */
int is_cmyk;			/* признак 4-красочного изображения; */
int miniswhite;			/* признак негативного изображения. */

/* Определение базовых параметров командной строки для всех фильтров. */
static struct option const base_long_options[] =
{
	{"help", no_argument, NULL, 'H'},
	{"version", no_argument, NULL, 'V'},
	{"pid", required_argument, NULL, 'p'},
	{"index", required_argument, NULL, 'i'},
	{"width", required_argument, NULL, 'w'},
	{"height", required_argument, NULL, 'h'},
	{"hres", required_argument, NULL, 'x'},
	{"vres", required_argument, NULL, 'y'},
	{"cmyk", no_argument, NULL, 'c'},
	{"density", no_argument, NULL, 'D'},
	{"intensity", no_argument, NULL, 'I'},
	{"verbose", no_argument, NULL, 'v'},
	{NULL, 0, NULL, 0}
};

/**
 * Возвращает имя временного файла для записи слоя изображения
 * с именем, соответствующим указанным параметрам: суффиксу
 * фильтра #fsuf и номеру цветового канала #color_idx.
 */
const char *
get_tmp_filter_file_name( const char *fsuf, int color_idx )
{
	if ( !pid ) {
		fprintf( stderr, "%s: Can't create temp. file: parent PID isn't set\n",
				 program_name );
		return NULL;
	}
	
	/* Получить полное имя файла по номеру процесса, номера фильтра и
	 * номеру цветового канала.
	 */
	return get_tmp_file_name( fsuf, pid, fidx, color_idx );
}

/* Вывод краткой справки и завершение работы с указанным кодом возврата. */
void
usage (int status, usage_header_f usage_header, usage_params_f usage_params)
{
  
  if (usage_header != NULL) {
	usage_header(stdout);
  }

  printf (_("\n\
\n\
Copyright (C) 2007 Yuri V. Kuznetsov, Paul Wolneykien.\n\
\n\
This program is free software: you can redistribute it and/or modify\n\
it under the terms of the GNU General Public License as published by\n\
the Free Software Foundation, either version 3 of the License, or\n\
(at your option) any later version. See http://www.gnu.org/licenses/\n\
for details.\n\n"));
  printf (_("Usage: %s [OPTION]...\n"), program_name);
  printf (_("\
Options:\n\
  -p PID, --pid=PID		PID of the parent process\n\
  -i IDX, --index=IDX		filter index\n\
  -w WIDTH, --width=WIDTH	RAW image width\n\
  -h HEIGHT, --height=HEIGHT	and height\n\
  -x HRES, --hres=HRES		RAW image horizontal\n\
  -y VRES, --vres=VRES		and vertical resolution\n\
  -c, --cmyk			RAW image is CMYK\n\
  -D, --density			input and output data is DENSITY \n\
                                values\n\
  -I, --intensity		intput and output data is INTENSITY\n\
                                values\n\
  -H, --help			display this help and exit\n\
  -V, --version			output version information and exit\n\
"));

  if (usage_params != NULL) {
	usage_params(stdout);
  }

  printf (_("\n"));

  exit (status);
}

/* Определение количества параметров коммандной строки, определённых в
 * массиве. */
int
options_count(struct option const *opts)
{

	int c = 0;
	while (opts->name != NULL || opts->has_arg != 0
		|| opts->flag !=  NULL || opts->val != 0) {
		opts++;
		c++;
	}

	return c;

}

/* Стандартная функция разбора аргументов коммандной строки для фильтра.
 * Специальные параметры передаются с использованием длинных имён.
 */
int
decode_switches (int argc, char **argv, int error_code,
		 struct option const *long_options,
		 char ***option_vars,
		 usage_header_f usage_header, usage_params_f usage_params)
{
  int c;
  char *endptr;
  int option_index;

  /* Признак передачи яркостных значений. */
  int want_intensity = 0;

  /* Признак передачи значений оптической плотности. */
  int want_density = 0;

  /* Размер массивов, определяющих набор базовых и дополнительных
   * аргументов коммандной строки соответственно. */
  int base_options_count;
  int special_options_count;

  /* Буфер для хранения описаний аргументов коммандной строки. */
  struct option *all_options = NULL;

  /* Установка значений по умолчанию.  */
  is_cmyk = 0;
  miniswhite = 0;
  width = 0;
  height = 0;
  vres = 0;
  hres = 0;
  pid = 0;
  fidx = 0;
  want_verbose = 0;

  /* Подсчёт количества базовых аргументов. */
  base_options_count = options_count(base_long_options);

  /* Подсчёт количества дополнительных аргументов. */
  special_options_count = options_count(long_options);

  /* Функция аварийной очистки занятых ресурсов. */
  void cleanup() {
	if (all_options != NULL) {
		free(all_options);
	}
  }

  /* Инициализация функции аварийной очистки. */
  push_cleanup(cleanup);

  /* Выделение памяти для совместного хранения описаний аргументов. */
  all_options = calloc(base_options_count + special_options_count + 1, sizeof(struct option));

  /* Копирование описание базовых аргументов в буфер. */
  memcpy(all_options, base_long_options, base_options_count*sizeof(struct option));
  /* Следом помещается описание специальных агрументов. */
  memcpy(all_options + base_options_count, long_options, (special_options_count + 1)*sizeof(struct option));

  /* Последовательный перебор базовых параметров. */
  while ((c = getopt_long (argc, argv,
		"p:"  /* номер основного программного процесса; */
	   	"i:"  /* номер фильтра; */
		"w:"  /* ширина изображения; */
		"h:"  /* высота изображения; */
		"x:"  /* разрешение по горизонтали; */
		"y:"  /* разрешение по вертикали; */
		"c"  /* признак 4-красочного изображения; */
		"D"	/* признак передачи значений плотности; */
		"I"	/* признак передачи яркостных значений; */
		"H"	/* вывод краткой справки; */
		"v"	/* режим повышенной информативности; */
		"V",	/* вывод информации о версии. */
		all_options, &option_index)) >= 0)
    {
      /* Иентификация параметра по ключу. */
      switch (c)
	{
		
	/* Номер основного программного процесса. Преобразование строкового
	 * представления в числовое. Выход, в случае ошибки преобразования.
         */
	case 'p':
		pid = strtoul(optarg, &endptr, 0);
		if (errno == ERANGE || *endptr != '\0') {
			fprintf(stderr, "%s: PID value is invalid.\n", program_name);
			exit(error_code);
		}
		break;

	/* Ширина изображения. Преобразование строкового  представления в 
	 * числовое. Выход, в случае ошибки преобразования.
	 */
	case 'w':
		width = strtoul(optarg, &endptr, 0);
		if (errno == ERANGE || *endptr != '\0') {
			fprintf(stderr, "%s: Width value is invalid.\n", program_name);
			exit(error_code);
		}
		break;
	
	/* Высота изображения. Преобразование строкового  представления в 
	 * числовое. Выход, в случае ошибки преобразования.
	 */	
	case 'h':
		height = strtoul(optarg, &endptr, 0);
		if (errno == ERANGE || *endptr != '\0') {
			fprintf(stderr, "%s: Height value is invalid.\n", program_name);
			exit(error_code);
		}
		break;

	/* Разрешение по горизонтали. Преобразование строкового  представления в 
	 * числовое. Выход, в случае ошибки преобразования.
	 */
	case 'x':
		hres = strtod(optarg, &endptr);
		if (errno == ERANGE || *endptr != '\0') {
			fprintf(stderr, "%s: HRES value is invalid.\n", program_name);
			exit(error_code);
		}
		break;

	/* Разрешение по вертикали. Преобразование строкового  представления в 
	 * числовое. Выход, в случае ошибки преобразования.
	 */
	case 'y':
		vres = strtod(optarg, &endptr);
		if (errno == ERANGE || *endptr != '\0') {
			fprintf(stderr, "%s: VRES value is invalid.\n", program_name);
			exit(error_code);
		}
		break;

	/* Признак 4-красочного изображения. Если явным образом не указан
	 * признак яркостных значений, то за обозначение пробела (незапечатанной
         * области, принимается 0.
	 */
	case 'c':
		is_cmyk = 1;
		if (!want_intensity)
			miniswhite = 1;
		break;

	/* Признак значений плотности. За обозначение пробела (незапечатанной
         * области, принимается 0. Выход, если одновременно был указан
	 * противоположный признак.
	 */
	case 'D':
		if (!want_intensity) {
			miniswhite = 1;
			want_density = 1;
		} else {
			fprintf(stderr, "INTNSITY and DENSITY are mutually exclusive options\n");
			exit(error_code);
		}
		break;

	/* Признак яркостных значений. За обозначение пробела (незапечатанной
         * области, принимается 0. Выход, если одновременно был указан
	 * противоположный признак.
	 */
	case 'I':
		if (!want_density) {
			miniswhite = 0;
			want_intensity = 1;
		} else {
			fprintf(stderr, "INTNSITY and DENSITY are mutually exclusive options\n");
			exit(error_code);
		}
		break;

	/* Номер фильтра. Преобразование строкового представления номера в
	 * числовое. Выход с признаком ошибки, в случае ошибки преобразования.
	 */
	case 'i':
		fidx = strtoul(optarg, &endptr, 0);
		if (errno == ERANGE || *endptr != '\0') {
			fprintf(stderr, "%s: INDEX value is invalid.\n", program_name);
			exit(error_code);
		}
		break;

	/* Включение режима повышенной информативности. */
	case 'v':
		want_verbose = 1;
		break;
		
	/* Вывод информации о версии и завершение работы. */
	case 'V':
	  printf ("%s filter from %s %s\n", program_name, PACKAGE, VERSION);
	  exit (0);

	/* Вывод краткой справки и завершение работы. */
	case 'H':
	  usage (0, usage_header, usage_params);

        case '\0':
	  if (option_vars != NULL && option_vars[option_index - base_options_count] != NULL) {
			*option_vars[option_index - base_options_count] = optarg;
	  }
	  break;

	/* Если ключ параметра не был идентифицирован, то производится вывод
	 * краткой справки и завершение работы с признаком ошибки.
	 */
	default:
	  usage (error_code, usage_header, usage_params);
	}
    }

   /* Освобождение занятых ресурсов. */
   (*pop_cleanup())();

  /* Возвращения номера первого не обработанного аргумента. */
  return optind;
}

/* Передать строку изображения из указанного буфера, состоящую из отсчётов
 * указанного размера и указанного количества таких отсчётов, в выходной поток
 * на дальнейшую обработку.
 */
void
write_outbuf(char *outbuf, size_t ss, size_t len) {

	size_t wt;

	wt = fwritesmp(outbuf, ss, len, stdout, miniswhite, NULL);
	if (wt < len) {
		fprintf(stderr, "%s: Failed to transfer scanline data further\n", program_name);
		/* Выход с признаком ошибки, если запись данных в поток
		 * завершилась ошибкой.
		 */
		exit(EXIT_FAILURE);
	}

}
