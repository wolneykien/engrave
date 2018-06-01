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
 *
 * Программа для адаптивного растрирования полутоновых оригиналов.
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>

#include "system.h"
#include <getopt.h>
#include <tiff.h>
#include <tiffio.h>
#include <math.h>
#include "misc.h"	/* Вспомогательные функции */

#ifdef WITH_PDFWRITER
#include "pdfwriter.h"
#endif

#define EXIT_FAILURE 1

/* Имя программы, указанное в коммандной строке */
char *program_name;

/* Коды возврата функции getopt_long */
enum {DUMMY_KEY=129
     ,BRIEF_KEY
};

/* Переменные, определяющие поведение программы. */

/* Признак запрета на вывод информации в консоль. */
int want_quiet;
/* Признак режима вывода с повышенной информативностью. */
int want_verbose;
/* Признак завершения работы  в случае обнаружения ошибки при
 * обработке файла. */
int exit_on_error;

/* Параметры файла конечного изображения. */
char output_name[MAXLINE];	/* Имя файла. */
char suffix[256];	/* Суффикс имени. */

/* Признак режима работы с неформатированными изобразительными данными.
 * Флаг --raw */
int is_raw;

/* Параметры неформатированного изображения */
uint32 width;	/* Ширина в пикселях */
uint32 height;	/* Высота в пикселях */
int miniswhite;	/* Признак принятия белого за 0 */
float hres;	/* Разрешение по горизонтали в точках на дюйм */
float vres;	/* Разрешение по вертикали в точках на дюйм */
int is_cmyk;	/* Признак наличия информации о 4 красках */

/* Признаки включения в результирующую программу фрагментов управления
 * отдельными красителями. */
int want_c;
int want_m;
int want_y;
int want_k;

/* Управление выходной изобразительной информацией */
/* Признак режима вывода значений оптической плотности */
int want_density;
/* Признак режима вывода яркости */
int want_intensity;

/* Набор коммандных строк фильтров; */
char filter[MAXLINE] = "";

/* Директория с исполняемыми файлами фильтров */
char filterdir[MAXLINE];
char psdir[MAXLINE]; /* Директория с библиотечными PostScript файлами */

/* Параметры файла уменьшенной копии изображения */
int want_preview;	/* Признак записи уменьшенной копии. */

/* Признак тестового режима. */
int want_test_run = 0;

/* Формат */
typedef enum { EPS_FMT, TIFF_FMT, PDF_FMT } outformat_t;
outformat_t outformat = EPS_FMT;

/* Задание списка параметров коммандной строки */
static struct option const long_options[] =
{
	{"quiet", no_argument, NULL, 'q'},
	{"silent", no_argument, NULL, 'q'},
	{"verbose", no_argument, NULL, 'v'},
	{"raw", no_argument, NULL, 'r'},
	{"help", no_argument, NULL, 'H'},
	{"version", no_argument, NULL, 'V'},
	{"width", required_argument, NULL, 'w'},
	{"height", required_argument, NULL, 'h'},
	{"hres", required_argument, NULL, 'x'},
	{"vres", required_argument, NULL, 'y'},
	{"cmyk", optional_argument, NULL, 'c'},
	{"density", no_argument, NULL, 'D'},
	{"intensity", no_argument, NULL, 'I'},
	{"filter", required_argument, NULL, 'f'},
	{"filter-path", required_argument, NULL, 'F'},
	{"ps-path", required_argument, NULL, 'P'},
	{"output", optional_argument, NULL, 'o'},
	{"output-suffix", required_argument, NULL, 'O'},
	{"preview", no_argument, NULL, 'p'},
	{"test-run", no_argument, NULL, 'T'},
	{"format", required_argument, NULL, 't'},
	{NULL, 0, NULL, 0}
};

/* Функция для включения содержимого указанного файла в выходной файл.
 * Опционально, файл может быть удалён. */
static void
dump_file( FILE *out, const char *fn )
{
	FILE *f = NULL;
	static char str[MAXLINE];

	/* Вспомогательная функция для закрытия файла в случае
	 * аварийной ситуации. */
	void cleanup() {
		if (f != NULL) {
			fclose(f);
			f = NULL;
		}
	}

	/* Регистирование функции закрытия файла в очереди очистки */
	push_cleanup(cleanup);

	/* Открытие указанного файла */
	f = fopen(fn, "r");
	if (f != NULL) { /* Если файл открыт удачно */
		if (want_verbose) /* Вывод сообщения в информативном режиме */
			fprintf(stderr, "Including file %s\n", fn);
		/* Копирование строк файла в выходной поток */
		while (fgets(str, sizeof(str), f) != NULL)
			fputs(str, out);

		/* Закрытие файла */
		fclose(f);
	}

	/* Указываем, что аварийной ситуации не произошло: */
	f = NULL;	/* очишаем указатель на файл; */
	errno = 0;	/* и уровень ошибок; */ 
	/* снимаем функцию аварийного закрытия со стека. */
	(*pop_cleanup())();
}

/* Копирование горизонтального ряда пикселов с их пропуском для
 * изменения масштаба.
 *
 * Аргументы:
 * buf -- указатель на строку исходного изображения;
 * ss -- размер пикселя в байтах;
 * width -- ширина исходного изображения в пикселях;
 * thumbnail_buf -- указатель на буфер назначения;
 * thumbnail_width -- ширина уменьшенной копии изображения. */
void get_thumbnail_line(char *buf, size_t ss, size_t width, \
			char *thumbnail_buf, size_t thumbnail_width) {

	double x = 0;
	double step;
	size_t i;
	int sx;

	/* Вычисление шага выборки. */
	step = (width - 1)/(double)(thumbnail_width - 1);

	/* Копирование блоков пикселей. */
	for (sx = 0; sx < thumbnail_width; sx++) {
		i = ss*rint(x);
		x += step;
		memcpy(thumbnail_buf, buf+i, ss);
		thumbnail_buf += ss;
	}
}

/* Петачть краткой справки с последующим выходом с указанным кодом
 * завершения. */
void
usage (int status)
{
  printf (_("%s - \
engrave - Adaptive screening of CT images\n\
\n\
Copyright (C) 2018 Yuri V. Kuznetsov, Paul A. Wolneykien.\n\
\n\
This program is free software: you can redistribute it and/or modify\n\
it under the terms of the GNU Affero General Public License as\n\
published by the Free Software Foundation, either version 3 of the\n\
License, or (at your option) any later version.\n\
See http://www.gnu.org/licenses/ for details.\n\n"), program_name);
  printf (_("Usage: %s [OPTIONS] -f FILTER [option] [-f FILTER [option]...] [FILE]\n"), \
	  program_name);
  printf (_("\n\
Options:\n\
  -r, --raw			process RAW image stream\n\
  -w WIDTH, --width=WIDTH	RAW image width\n\
  -h HEIGHT, --height=HEIGHT	and height\n\
  -x HRES, --hres=HRES		RAW image horizontal\n\
  -y VRES, --vres=VRES		and vertical resolution\n\
  -c [CMYK], --cmyk[=CMYK]	image is CMYK; optionally selects\n\
                                individual colorants;\n\
  -D, --density			input and output data is DENSITY\n\
                                values\n\
  -I, --intensity		intput and output data is INTENSITY\n\
                                values\n\
  -F FPATH --filter-path=FPATH	path to filters, default is '%s'\n\
  -P PSPATH --ps-path=PSPATH	path to PostScript library files,\n\
                                default is '%s'\n\
  -o [NAME], --output[=NAME]	write output to file [NAME] (default\n\
                                <NAME>.<SUF>)\n\
  -O SUF, --output-suffix=SUF	write output file to <NAME>.SUF \n\
                                (default 'eps')\n\
  -p, --preview			add preview image to the EPS\n\
  -T, --test-run        keep temporary files\n\
  -t FMT, --format=FMT  output format (eps, tiff)\n\
  -H, --help			display this help and exit\n\
  -v, --verbose			verbose message output\n\
  -V, --version			output version information and exit\n\
\n\
Filter chain:\n\
  -f FILTER [options], \n\
  --filter=FILTER [options]     process image through FILTER [filter\n\
                                options].\n\
"), filterdir, psdir);
  exit (status);
}

/* Обработка аргументов коммандной строки каждого из
 * указанных фильтров. */
int
decode_filter_switches(int argc, char **argv)
{

  /* Переменная optind указывает на первый необработанный
   * аргумент. */
  int i = optind;
  char *arg;

  /* Разбор аргументов коммандной строки. */

  /* Копирование имени фильтра в начало командной строки. */
  strcpy(filter, argv[i - 1]);

  /* Последовательная обработка аргументов. */
  while (i < argc) {
	arg = argv[i];
	/* Проверка, является ли аргумент ключевым */
	if (arg[0] != '-') {
		/* Завершение разбора комманд фильтрации. */
		strcat(filter, "\n");
		break;
	} else if (strcmp(arg, "-f") == 0) {
		/* Символ перевода строки разделяет комманды фильтрации. */
		strcat(filter, "\n");
		/* Проверка на наличие следующего аргумента. */
		if (i == argc) {
			/* Вывод сообщения об ошибке */
			fprintf(stderr, "No filter name specified\n");
			usage(EXIT_FAILURE); /* Печать справки и выход */
		} else {
			/* Переход к следующему аргументу. */
			i++;
		}
		/* Имя сделующего фильтра добавляется
		 * в начало новой строки. */
		strcat(filter, argv[i]);
	} else {
		strcat(filter, " "); /* Дополнение строки пробелом */
		strcat(filter, arg); /* Добавление аргумента */
	}

	/* Переход к следующему аргументу. */
	i++;
  }

  return i;
}

/* Установка значений переменных в соответствие с аргументами
 * коммандной строки. Функция возвращает номер первого аргумента,
 * не являющегося ключом. */
int
decode_switches (int argc, char **argv)
{
  int c;
  int option_index;
  char *endptr;

  /* Задаются значения переменных по умолчанию. */
  is_cmyk = 0;
  want_c = 0;
  want_m = 0;
  want_y = 0;
  want_k = 0;
  want_density = 0;
  want_intensity = 0;
  width = 0;
  height = 0;
  vres = 0;
  hres = 0;
  miniswhite = 0;
  want_verbose = 0;
  want_quiet = 0;
  is_raw = 0;
  filter[0] = '\0';
  suffix[0] = '\0';
  output_name[0] = '\0';
  want_preview = 0;
  want_test_run = 0;
  outformat = EPS_FMT;

  /* Перебор параметров коммандной строки с помощью функции getopt_long. */
  while ((c = getopt_long (argc, argv, 
			   "r"  /* raw */
			   "f:"  /* filter */
			   "F:"  /* filter path */
			   "P:"	/* pslib path */
			   "q"  /* quiet */
			   "v"  /* verbose */
			   "w:"  /* width */
			   "h:"  /* height */
			   "x:"  /* hres */
			   "y:"  /* vres */
			   "c::"  /* cmyk */
			   "D"	/* density */
			   "I" /* intensity */
			   "H"	/* help */
			   "V"	/* version */
			   "o::" /* output to file */
			   "O:" /* output suffix */
			   "p"  /* add preview */
			   "T"  /* test run */
			   "t:", /* output format */
			   long_options, &option_index)) != EOF)
    {
      switch (c) /* Анализ буквенного кода параметра. */
	{

	/* Выбор режима работы с неформатированным изображением. */
	case 'r':
		is_raw = 1;
		break;

	/* Выбор директории с исполняемыми файлами фильтров. */
	case 'F':
		snprintf(filterdir, sizeof(filterdir), optarg);
		snprintf(psdir, sizeof(psdir), optarg);
		break;

	/* Выбор директории с библиотечными PostScript файлами. */
	case 'P':
		snprintf(psdir, sizeof(psdir), optarg);
		break;

	/* Выбор записи в файл. */
	case 'o':
		if (optarg != NULL)
			strcpy(output_name, optarg);
		break;

	/* Выбор суффикса файла назначения. */
	case 'O':
		strcpy(suffix, optarg);
		break;
	
	/* Выбор добавления уменьшенной копии в файл. */
	case 'p':
		want_preview = 1;
		break;

	/* Выбор режима пониженной информативности сообщений. */
	case 'q':
		want_quiet = 1;
		break;

	/* Выбор режима повышенной информативности сообщений. */
	case 'v':
		want_verbose = 1;
		break;
		
	/* Задание ширины изображения. */
	case 'w':
		/* Попытка преобразования символов в число. */
		width = strtoul(optarg, &endptr, 0);
		/* Выход с выводом сообщения об ошибке в случае неудачи. */
		if (errno == ERANGE || *endptr != '\0') {
			fprintf(stderr, "%s", "Width value is invalid.\n");
			exit(EXIT_FAILURE);
		}
		break;
		
	/* Задание высоты изображения. */
	case 'h':
		/* Попытка преобразования символов в число. */
		height = strtoul(optarg, &endptr, 0);
		/* Выход с выводом сообщения об ошибке в случае неудачи. */
		if (errno == ERANGE || *endptr != '\0') {
			fprintf(stderr, "%s", "Height value is invalid.\n");
			exit(EXIT_FAILURE);
		}
		break;

	/* Задание горизонтального разрешения изображения. */
	case 'x':
		/* Попытка преобразования символов в число. */
		hres = strtod(optarg, &endptr);
		/* Выход с выводом сообщения об ошибке в случае неудачи. */
		if (errno == ERANGE || *endptr != '\0') {
			fprintf(stderr, "%s", "HRES value is invalid.\n");
			exit(EXIT_FAILURE);
		}
		break;

	/* Задание вертикального разрешения изображения. */
	case 'y':
		/* Попытка преобразования символов в число. */
		vres = strtod(optarg, &endptr);
		/* Выход с выводом сообщения об ошибке в случае неудачи. */
		if (errno == ERANGE || *endptr != '\0') {
			fprintf(stderr, "%s", "VRES value is invalid.\n");
			exit(EXIT_FAILURE);
		}
		break;

	/* Выбор режима четырёхкрасочного изображения. */
	case 'c':
		is_cmyk = 1;
		/* Принятие белого за 0, если не указано обратного. */
		if (!want_intensity) {
			miniswhite = 1;
		}
		if (optarg != NULL) {
		  if (strchr(optarg, 'C') != NULL) {
		    want_c = 1;
		  }
		  if (strchr(optarg, 'M') != NULL) {
		    want_m = 1;
		  }
		  if (strchr(optarg, 'Y') != NULL) {
		    want_y = 1;
		  }
		  if (strchr(optarg, 'K') != NULL) {
		    want_k = 1;
		  }
		}
		break;

	/* Выбор режима вывода значений оптической плотности. */
	case 'D':
		if (!want_intensity) {
			miniswhite = 1; /* Принятие белого за 0. */
			want_density = 1;
		} else {
		/* Сообщение об ошибке, если обнаружен конфликт параметров. */
			fprintf(stderr, "INTNSITY and DENSITY are mutually exclusive options\n");
			exit(EXIT_FAILURE); /* Выход */
		}
		break;

	/* Выбор режима вывода значений яркости. */
	case 'I':
		if (!want_density) {
			miniswhite = 0; /* Принятие чёрного за 0. */
			want_intensity = 1;
		} else {
		/* Сообщение об ошибке, если обнаружен конфликт параметров. */
			fprintf(stderr, "INTNSITY and DENSITY are mutually exclusive options\n");
			exit(EXIT_FAILURE); /* Выход */
		}
		break;

	/* Печать номера версии и выход. */
	case 'V':
	  printf ("engrave %s\n", VERSION);
	  exit (0);

	/* Печать краткой справки и выход. */
	case 'H':
	  usage (0);

	/* Разбор коммандных строк фильтров.
	 * На этом разбор аргументов завершается.
	 * Функция разбора аргументов фильтра возвращает
	 * номер первого неключевого аргумента. */
	case 'f':
		return decode_filter_switches(argc, argv);

	/*
	 * Тестовый прогон (не удалять временные файлы).
	 */
	case 'T':
	  want_test_run = 1;
	  break;

	case 't':
	  if ( 0 == strcmp( optarg, "eps" ) ||
	  	   0 == strcmp( optarg, "EPS" ) )
	  	{
	  		outformat = EPS_FMT;
			break;
	  	}
	  if ( 0 == strcmp( optarg, "tiff" ) ||
	  	   0 == strcmp( optarg, "TIFF" ) )
	  	{
	  		outformat = TIFF_FMT;
			break;
	  	}
	  if ( 0 == strcmp( optarg, "pdf" ) ||
	  	   0 == strcmp( optarg, "PDF" ) )
	  	{
	  		outformat = PDF_FMT;
			break;
	  	}

	/* Если аргумент не был распознан, он признаётся ошибочным,
	 * выводится краткая справка и производится выход
	 * с кодом ошибки. */
	default:
	  usage (EXIT_FAILURE);
	}
    }

  /* Возврат значения внешней переменной optind, указывающей
   * на номер следующего необработанного аргумента. */
  return optind;
}

/* Главная функция. */
int
main (int argc, char **argv)
{

  int opt_r;	/* Результат вызова decode_options(). */
  int retc;	/* Код возврата из функции обработки. */

  /* Получение имени программы. */
  program_name = argv[0];

  char pdirbuf[1024];
  snprintf(pdirbuf, sizeof(pdirbuf), "%s", argv[0]);
  dirname(pdirbuf);

  if (strcmp(pdirbuf, argv[0]) == 0) {
    /* Запуск из текущей директории (под Win)? */
    pdirbuf[0] = '\0';
  }

  /* Для задания директорий по умолчанию используются значения констант.
     Если путь относительный, то добавляется путь к программе. */

#ifdef FILTERS
#  ifndef __MINGW32__
  if (strncmp(FILTERS, "/", 1) == 0)
    snprintf(filterdir, sizeof(filterdir), "%s", FILTERS);
  else
    snprintf(filterdir, sizeof(filterdir), "%s/%s", pdirbuf, FILTERS);
#  else
  if (strlen(pdirbuf) > 0)
    snprintf(filterdir, sizeof(filterdir), "%s\\%s", pdirbuf, FILTERS);
  else
    snprintf(filterdir, sizeof(filterdir), "%s", FILTERS);
#  endif
#else
  snprintf(filterdir, sizeof(filterdir), "%s", pdirbuf);
#endif

#ifdef PSLIB
#  ifndef __MINGW32__
  if (strncmp(PSLIB, "/", 1) == 0)
    snprintf(psdir, sizeof(psdir), "%s", PSLIB);
  else
    snprintf(psdir, sizeof(psdir), "%s/%s", pdirbuf, PSLIB);
#  else
  if (strlen(pdirbuf) > 0)
    snprintf(filterdir, sizeof(filterdir), "%s\\%s", pdirbuf, PSLIB);
  else
    snprintf(filterdir, sizeof(filterdir), "%s", PSLIB);
#  endif
#else
  snprintf(psdir, sizeof(psdir), "%s", pdirbuf);
#endif
  
  /* Разбор аргументов коммандной строки. */
  opt_r = decode_switches (argc, argv);

  /* Если не указано ни одного исходного файла,
   * то функция обработки вызывается с пустым аргументом,
   * означающим, что следует обрабатывать неформатированные
   * данные, подаваемые на стандартный вход. */
  if (opt_r == argc)
  	return process(NULL);

  /* Последовательная обработка файлов, указанных в коммандной строке. */
  while (opt_r < argc) {
  	retc = process(argv[opt_r]);
	if (retc && exit_on_error) {
		return retc;
	}

	/* Сброс имени выходного файла. */
	output_name[0] = '\0';

  	opt_r++;
  }

  return 0;
}

/* Вспомогательная функция.
 * Получение нового имени файла из старого,
 * путём замены суффикса. */
void
set_suffix(char *destname, const char *filename, const char *suffix)
{

  char *dot;

  dot = strrchr(filename, '.'); 
  if (dot != NULL)
  	dot[0] = '\0';
  snprintf(destname, MAXLINE, "%s.%s", filename, suffix);
  if (dot != NULL)
  	dot[0] = '.';
  
}

/* Вспомогательная функция.
 * Определение базового имени фильтра. */
void
filter_basename(char *filter_name, const char *a_filter)
{
	char *filter_tail;

	filter_tail = strchr(a_filter, ' ');
	if (filter_tail != NULL) {
		*filter_tail = '\0';
	}
	strcpy(filter_name, a_filter);
	if (filter_tail != NULL) {
		*filter_tail = ' ';
	}
}

struct output_ctx {
	FILE *output_file;
	void *pdfctx;
};

static void
close_output( struct output_ctx* ctx )
{
	if ( ctx ) {
		if ( ctx->output_file ) {
			if ( ctx->output_file != stdout ) {
				if ( fclose(ctx->output_file) && want_verbose ) {
					fprintf(stderr, "Failed to close the output file\n");
				}
			}
			ctx->output_file = NULL;
		}
		if ( ctx->pdfctx ) {
			if ( pdf_close( ctx->pdfctx ) && want_verbose ) {
				fprintf(stderr, "Failed to close the output PDF file\n");
			}
			ctx->pdfctx = NULL;
		}
		free( ctx );
	}
}

/* Функция проверки корректности ввода параметров и
 * подготовки файлов к работе. */
static int
prepare_output( const char *file_name, char *output_name,
				struct output_ctx **outctx )
{
  if ( outformat == TIFF_FMT ) return 0;
	
  /* Подготовка файлов.
   * Определение имён в соответствии с указанными суффиксами. */

  /* Определение файла назначения.
   * Если имя или суффикс не указаны, то используются
   * значения по умолчанию. */
	
   if (suffix[0] == '\0') {
	   switch ( outformat ) {
	   case PDF_FMT:
		   strcpy(suffix, "pdf");
		   break;
	   default:
		   strcpy(suffix, "eps");
	   }
   }
   
   if (strlen(output_name) == 0) {
	   if (file_name != NULL) {
		   set_suffix(output_name, file_name, suffix);
	   } else {
		   set_suffix(output_name, "output", suffix);
	   }
   }

   *outctx = malloc( sizeof(*outctx) );
   if ( !*outctx ) return EXIT_FAILURE;
   (*outctx)->output_file = NULL;
   (*outctx)->pdfctx = NULL;
   
   switch ( outformat ) {
   case PDF_FMT:
	   (*outctx)->pdfctx =
		   pdf_open_file( output_name,
						  ((double) width / (double) hres) * 72,
						  ((double) height / (double) vres) * 72 );
	   if ( !(*outctx)->pdfctx )
		   return EXIT_FAILURE;
	   break;
   default:
	   if (strcmp(output_name, "-") == 0) {
		   (*outctx)->output_file = stdout;
	   } else {
		   if (want_preview) {
			   strcat(output_name, "~");
		   }
		   (*outctx)->output_file = fopen(output_name, "w");
		   if ( !(*outctx)->output_file ) {
			   return EXIT_FAILURE;
		   }
	   }
   }

   if (want_verbose) {
	   fprintf(stderr, "\nProcessing file %s\nOutput file is %s\n",	\
			   (file_name != NULL ? file_name : "-"),				\
			   (output_name != NULL ? output_name : "-"));
   }

   /* Признак успешного завершения функции. */
   return 0;
}

/* Вспомогательная функция для открытия файла форматированного
 * изображения в формате TIFF. */
TIFF *
open_tif_file(char *file_name, uint16 *phm, tsample_t *spp, uint16 *r_unit, uint16 *tiff_planar)
{

  TIFF *tif;

  if (!is_raw) {
  	/* Открытие файла изображения в формате TIFF. */
	  tif = TIFFOpen(file_name, "r");
	  /* Сообщение об ошибке если не удалось открыть файл. */
	  if (tif == NULL) {
		  fprintf(stderr, "Error open TIFF file %s\n", file_name);
		  exit(EXIT_FAILURE); /* Выход */
	  }

	  /* Разбор тегов изображения и запись их в переменные. */
	  TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
	  TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
	  TIFFGetField(tif, TIFFTAG_XRESOLUTION, &hres);
	  TIFFGetField(tif, TIFFTAG_YRESOLUTION, &vres);
	  TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, phm);
	  TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, spp);
	  TIFFGetField(tif, TIFFTAG_PLANARCONFIG, tiff_planar);

	  /* Установка режима работы в соответствие
	   * со значениями тегов. */
	  switch (*spp) {
		  case 1: /* В случае 1 байта на пиксел. */
			  if (*phm == PHOTOMETRIC_MINISWHITE) {
				  miniswhite = 1;
			/* Изображение не может быть многокрасочным. */
				  is_cmyk = 0;
			  }
			  break;
		  /* 4 байта на пиксель означают 4 красочное изображение. */
		  case 4: 
			  if (*phm == PHOTOMETRIC_SEPARATED) {
				  miniswhite = 1;
				  is_cmyk = 1;
			  } else {
			  /* Но это также может быть и неизвестное цветовое
			   * пространство. */
				  fprintf(stderr, "Four channel image is not separated...Exit\n");
				  exit(EXIT_FAILURE);
			  }
			  break;
		  default:
		  	/* Любое другое количество отсчётов на пиксель
			 * считается ошибочным. */
			  fprintf(stderr, "Illegal samples per pixel (number of channels): %i\n", *spp);
			  exit(EXIT_FAILURE);
	  }

	  /* Определение разрешения изображения.
	   * Определение единицы измерения разрешения. */
	  TIFFGetField(tif, TIFFTAG_RESOLUTIONUNIT, r_unit);
	  /* Вычисление разрешения в точках на дюйм, в том случае
	   * если оно указано в точках на сантиметр. */
	  if (*r_unit == RESUNIT_CENTIMETER) {
		  hres /= 2.54;
		  vres /= 2.54;
	  }
  }

  return tif;

}

void
copy_file(FILE *dst, FILE *src)
{

	/* Промежуточный буфер и контрольные счётчики для операций
	 * вода/вывода. */
	char buf[4096];
	int rd;
	int wt;
	long total = 0;

	/* Копирование файла через промежуточный буфер. */
	rd = fread(buf, 1, sizeof(buf), src);
	while (rd > 0) {
		wt = fwrite(buf, 1, rd, dst);
		total += wt;
		/* Если буфер был записан не целиком, то производится выход
	 	 * с признаком ошибки. */
		if (wt != rd) {
			fprintf(stderr, "Unable to write to file\n");
			exit(EXIT_FAILURE);
		}
		rd = fread(buf, 1, sizeof(buf), src);
	}
	if (want_verbose) {
	  fprintf(stderr, "%li bytes written\n", total);
	}
}

void
add_preview (char *psname, char *tiffname)
{

  	/* Структуры для хранения информации о файлах: */
	struct stat psstat;	/* о EPS файле; */
	struct stat tiffstat;	/* о TIFF файле. */
	
	/* Структура заголвка EPS-файла (EPSF). */
	struct header_s {
		unsigned char info[4];
		unsigned char psstart[4];
		unsigned char pslength[4];
		unsigned char mfstart[4];
		unsigned char mflength[4];
		unsigned char tiffstart[4];
		unsigned char tifflength[4];
		unsigned char check[2];
	} header;

	/* Указатели для доступа к исходным файлам. */
	FILE *eps = NULL;
	FILE *tif = NULL;

	/* Имя результирующего файла */
        char resname[MAXLINE] = "";

	/* Указатель для доступа к результирующиму файлу. */
	FILE *res = NULL;

	/* Контрольный счётчик для операции записи в файл. */
	int wt;

	/* Функция аварийной очистки занятых ресурсов. */
	void cleanup() {
	  if (eps != NULL) {
	    fclose(eps);
	    if (!unlink(psname) && want_verbose) {
	      fprintf(stderr, "Delete temporary file %s\n", psname);
	    }
	  }
	  if (tif != NULL) {
	    fclose(tif);
	    if (!want_test_run) {
	      if (!unlink(tiffname) && want_verbose) {
		fprintf(stderr, "Delete temporary file %s\n", tiffname);
	      }
	    } else {
	      fprintf(stderr, "Test run: temporary file %s not deleted.\n", tiffname);
	    }
	  }
	  if (res != NULL) {
	    fclose(res);
	  }
	}

	/* Инициализация механизма аварийного освобождения ресурсов. */
	push_cleanup(cleanup);

	/* Получение информации о EPS-файле. */
	if (stat(psname, &psstat)) {
		fprintf(stderr, "Can't get attributes of file %s\n", psname);
		exit(EXIT_FAILURE);	/* Выхлуод в случае ошибки. */
	}
	/* Получение информации о TIFF-файле. */
	if (stat(tiffname, &tiffstat)) {
		fprintf(stderr, "Can't get attributes of file %s\n", tiffname);
		exit(EXIT_FAILURE);	/* Выход в случае ошибки. */
	}

	/* Заполнение структуры заголовка файла в соответствие со
	 * спецификацией EPSF. */
	header.info[0] = 0xC5;
	header.info[1] = 0xD0;
	header.info[2] = 0xD3;
	header.info[3] = 0xC6;
	
	header.psstart[0] = sizeof(header);
	header.psstart[1] = 0;
	header.psstart[2] = 0;
	header.psstart[3] = 0;

	/* Использование полученной информации о файлах для заполнения
	 * заголовка. */
	header.tifflength[0] = (unsigned char)(tiffstat.st_size & 0xFF);
	header.tifflength[1] = (unsigned char)((tiffstat.st_size >> 8) & 0xFF);
	header.tifflength[2] = (unsigned char)((tiffstat.st_size >> 16) & 0xFF);
	header.tifflength[3] = (unsigned char)((tiffstat.st_size >> 24) & 0xFF);
	
	header.tiffstart[0] = (unsigned char)((psstat.st_size + sizeof(header)) & 0xFF);
	header.tiffstart[1] = (unsigned char)(((psstat.st_size + sizeof(header)) >> 8) & 0xFF);
	header.tiffstart[2] = (unsigned char)(((psstat.st_size + sizeof(header)) >> 16) & 0xFF);
	header.tiffstart[3] = (unsigned char)(((psstat.st_size + sizeof(header)) >> 24) & 0xFF);
	
	header.pslength[0] = (unsigned char)(psstat.st_size & 0xFF);
	header.pslength[1] = (unsigned char)((psstat.st_size >> 8) & 0xFF);
	header.pslength[2] = (unsigned char)((psstat.st_size >> 16) & 0xFF);
	header.pslength[3] = (unsigned char)((psstat.st_size >> 24) & 0xFF);

	header.mfstart[0] = 0;
	header.mfstart[1] = 0;
	header.mfstart[2] = 0;
	header.mfstart[3] = 0;

	header.mflength[0] = 0;
	header.mflength[1] = 0;
	header.mflength[2] = 0;
	header.mflength[3] = 0;

	/* Установка признака отсутствия контрольной суммы загоовка. */
	header.check[0] = 0xFF;
	header.check[1] = 0xFF;

	/* Открытие файла уменьшенной копии. */
	tif = fopen(tiffname, "r");
	if (tif == NULL) {
		fprintf(stderr, "Can't open file %s\n", tiffname);
		exit(EXIT_FAILURE);	/* Выход в случае ошибки. */
	}

	/* Открытие PostScript-файла. */
	eps = fopen(psname, "r");
	if (eps == NULL) {
		fprintf(stderr, "Can't open file %s\n", psname);
		exit(EXIT_FAILURE);	/* Выход в случае ошибки. */
	}
	
	/* Уменьшение имени файла на один символ, поскольку ранее к нему
	 * был добавлен символ '~', чтобы именть возможность использовать
	 * указанное в коммандной строке имя в качестве имени результирующего
	 * файла. */
	strncpy(resname, psname, strlen(psname) - 1);

	/* Открытие результирующего файла на запись */
	res = fopen(resname, "w+");
	if (res == NULL) {
		fprintf(stderr, "Can't open file %s\n", resname);
		exit(EXIT_FAILURE);	/* Выход в случае ошибки. */
	}

	/* Запись заголовка в результирующий файл. */
	if (want_verbose) {
		fprintf(stderr, "Writing EPSF header... \n");
	}
	wt = fwrite(&header, 1, sizeof(header), res);

	/* Если загловок был записан не целиком, то производится выход
	 * с признаком ошибки. */
	if (wt < sizeof(header)) {
		fprintf(stderr, "Unable to write header to %s\n", resname);
		exit(EXIT_FAILURE);
	}

	/* Запись Post-Script-файла. */
	if (want_verbose) {
		fprintf(stderr, "Writing PS... \n");
	}
	copy_file(res, eps);

	/* Запись уменьшенной копии изображения. */
	if (want_verbose) {
		fprintf(stderr, "Writing preview... \n");
	}
	copy_file(res, tif);

	if (want_verbose) {
		fprintf(stderr, "Unlink files... \n");
	}

	/* Выполнение процедуры освобождения занятых ресурсов. */
	/* Удаление скопированных файлов. */
  	(*pop_cleanup())();

	if (want_verbose) {
		fprintf(stderr, "EPSF done.\n");
	}

}

/* Составление коммандной строки для вызова фильтров. */
void
parse_filters(char *f_cmd, int *filter_count, pid_t pid)
{

  /* Переменные для работы с коммандными строками фильтров. */
  char *next_filter;
  char *a_filter;
  char i_arg[32];
  char f_path[MAXLINE];
  char f_args[MAXLINE];

  char *outformat_str = NULL;

  /* Преобразование выбранного формата в строковое
     представление. */
  switch ( outformat ) {
  case EPS_FMT:
	  outformat_str = "eps";
	  break;
  case TIFF_FMT:
  case PDF_FMT:
	  outformat_str = "tiff";
	  break;
  default:
	  fprintf( stderr, "BUG: Unexpected output format: %d\n",
			   outformat );
	  exit(EXIT_FAILURE);
  }

  /* Формирование коммандной строки для вызова фильтров. */
  /* Задание общих для всех фильтров аргументов, определяющих
   * параметры обрабатываемого изображения межпроцессорной
   * коммуникации. */
  snprintf(f_args, sizeof(f_args), " -p %u -w %u -h %u -x %.2f -y %.2f -t %s", pid, width, height, hres, vres, outformat_str);
  
  /* Добавление признака 4 красочного изображения. */
  if (is_cmyk)
	strcat(f_args, " -c");
  /* Добавление признака негативного изображения. */
  if (want_density || !want_intensity && miniswhite)
	  strcat(f_args, " -D");
  else
	  strcat(f_args, " -I");

  /* Добавление признака повышенной информативности для фильтров, если он
   * задан для основной программы. */
  if (want_verbose) {
	strcat(f_args, " -v");
  }

  /* Начальная установка указателя. */
  next_filter = filter;

  /* Обрезка коммандной строки до нулевой длины. */
  f_cmd[0] = '\0';

  *filter_count = 0;
  
  /* Выделение части строки до символа перевода. */
  char *saveptr = NULL;
  a_filter = strtok_r(next_filter, "\n", &saveptr);
  while (a_filter != NULL) {
    /* Если коммандная строка уже содержит некоторые комманды,
     * в начало коммандной строки добавляется символ коммуникации. */
    if (strlen(f_cmd) > 0)
      strcat(f_cmd, " | ");
    
    /* Сборка коммандной строки. */
    strcpy(f_path, filterdir);
    pathcat(f_path, a_filter);
    strcat(f_cmd, f_path);
    strcat(f_cmd, f_args);
    
    /* Добавление ключа с номером фильтра. */
    snprintf(i_arg, sizeof(i_arg), " -i %u", (*filter_count)++);
    strcat(f_cmd, i_arg);

    /* Повторение операций со следующим фильтром. */
    a_filter = strtok_r(NULL, "\n", &saveptr);
  }

  /* TODO: Решить вопрос с "лишними" данными по-другому. */
#ifndef __MINGW32__
  strcat(f_cmd, " > /dev/null");
#endif
}

/* Функция для вывода заголовка PostScript-программы. */
void
ps_header(const char *file_name, FILE *output_file)
{

  /* Переменные для хранения текущего времени. */
  time_t cr_time;
  char cr_time_str[256];

  /* Переменные для работы с коммандными строками фильтров. */
  char *next_filter;
  char *a_filter;
  char filter_name[256];

  /* Переменая для составления пути к библиотечным PostScript-файлам. */
  char ps_path[MAXLINE];

  /* Определение текущего времени. */
  cr_time = time(NULL);
  /* Преобразование полученного знечения времени. */
  strftime(cr_time_str, sizeof(cr_time_str), "%Y-%m-%d %H:%M:%S", localtime(&cr_time));

  /* Вывод заголовка PostScript программы для построения
   * конечного изображения. */
  fprintf(output_file,
  	 "%%!PS-Adobe-3.0 EPSF-3.0\n"
	 "%%%%Creator: "PACKAGE" "VERSION". Adaptive Screening Technology.\n"
	 "%%%%Title: %s\n"
	 "%%%%CreationDate: %s"
	 "%%%%DocumentData: Clean7Bit\n"
	 "%%%%LanguageLevel: 2\n"
	 "%%%%Pages: 1\n"
	 "%%%%BoundingBox: 0 0 %.0f %.0f\n"
	 "%%%%DocumentProcessColors: %s\n"
	 "%%%%EndComments\n"
	 "%%%%BeginProlog\n"
	 "%% Use own dictionary to avoid conflicts\n"
	 "40 dict begin\n",
	 file_name,		/* Имя файла. */
	 cr_time_str,		/* Текущее время. */
	 (float) width/hres*72,		/* Ширина */
	 (float) height/vres*72,	/* и высота в пунктах. */
	 /* Название красителя. */
	 is_cmyk ? "Cyan Magenta Yellow Black" : "Black");

  /* Добавление библиотечных PostScript файлов в основную программу. */
  
  /* Начальная установка указателя. */
  next_filter = filter;
  
  /* Выделение части строки до символа перевода. */
  char *saveptr = NULL;
  a_filter = strtok_r(next_filter, "\n", &saveptr);
  while (a_filter != NULL) {
    filter_basename(filter_name, a_filter);
    strcpy(ps_path, psdir);
    pathcat(ps_path, filter_name);

    /* Копирование библиотечного PostScript файла в выходной поток. */
    dump_file(output_file, strcat(ps_path, ".ps"));

    /* Повторение операций со следующим фильтром. */
    a_filter = strtok_r(NULL, "\n", &saveptr);
  }

  /* Завершение заголовка PostScript программы. */
  fprintf(output_file,	"%%%%EndProlog\n"
	 		"%%%%Page: 1 1\n"
	 		"gsave\t%% Save grafics state\n"
	 		"true setoverprint\n");
}

/* Подготовка параметров файла уменьшенной копии. */
void
prepare_preview(char *thumbnail_name, pid_t pid, uint16 phm, int ss, uint16 tiff_planar, TIFF **thumbnail, char **thumbnail_buf, uint32 *thumbnail_width, uint32 *thumbnail_height)
{

  /* Открытие файла уменьшенной копии изображения на запись.
   * Обработка ошибок. */
  if (want_preview) {
  	  snprintf(thumbnail_name, MAXLINE, "%i.prv.tif", pid);
	  *thumbnail = TIFFOpen(thumbnail_name, "w");
	  if (thumbnail == NULL) {
		  fprintf(stderr, "Can't write thumbnail file %s\n", "");
		  exit(EXIT_FAILURE);
	  }
  }

  /* Вычисление размеров уменьшенной копии изображения исходя из того,
   * что её разрешение должно равняться 72 точкам на дюйм. */
  if (*thumbnail != NULL) {
	  if (hres > 72)
		  *thumbnail_width = rint(width*(72/hres));
	  else /* Уменьшения по ширине не требуется. */
		  *thumbnail_width = width;
	  if (vres > 72)
		  *thumbnail_height = rint(height*(72/vres));
	  else /* Уменьшения по высоте не требуется. */
		  *thumbnail_height = height;
	  
  	  /* Выделение памяти для хранения строк уменьшенной копии. */
	  *thumbnail_buf = _TIFFmalloc(ss * *thumbnail_width);
	  if (*thumbnail_buf == NULL) {
		  fprintf(stderr, "Thumbnail scanline buffer allocation failed\n");
		  exit(EXIT_FAILURE);
	  }

	  /* Задание тегов изображения уменьшенной копии. */
	  TIFFSetField(*thumbnail, TIFFTAG_PLANARCONFIG, tiff_planar);
	  TIFFSetField(*thumbnail, TIFFTAG_SAMPLESPERPIXEL, ss);
	  TIFFSetField(*thumbnail, TIFFTAG_BITSPERSAMPLE, 8);
	  TIFFSetField(*thumbnail, TIFFTAG_IMAGEWIDTH, *thumbnail_width);
	  TIFFSetField(*thumbnail, TIFFTAG_IMAGELENGTH, *thumbnail_height);
	  TIFFSetField(*thumbnail, TIFFTAG_XRESOLUTION, hres < 72 ? hres : 72);
	  TIFFSetField(*thumbnail, TIFFTAG_YRESOLUTION, vres < 72 ? vres : 72);
	  TIFFSetField(*thumbnail, TIFFTAG_PHOTOMETRIC, phm);
	  TIFFSetField(*thumbnail, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);

  }

}

static void
delete_temporary_filter_file( const char *fsuf, pid_t pid,
							  int fidx, int color_idx )
{
	const char *tmp_fn = NULL;
	
	tmp_fn = get_tmp_file_name( fsuf, pid, fidx, color_idx );
	if ( !tmp_fn ) {
		fprintf(stderr, "Unable to get the name of a filter "	\
				"temporary file\n");
	} else {
		if ( !want_test_run ) {
			if ( unlink(tmp_fn) ) {
				fprintf(stderr, "Unable to delete temporary file\n");
			} else if ( want_verbose ) {
				fprintf(stderr, "Delete temporary file %s\n", tmp_fn);
			}
		} else {
			fprintf(stderr,
					"Test run: temporary file %s not deleted.\n",
					tmp_fn);
		}
		free( tmp_fn ); tmp_fn = NULL;
	}
}

static void
use_temporary_filter_file( struct output_ctx *outctx,
						   const char *fsuf, pid_t pid,
						   int fidx, int color_idx )
{
	const char *tmp_fn = NULL;
	pdfcolor_t pdf_color;
	
	tmp_fn = get_tmp_file_name( fsuf, pid, fidx, color_idx );
	if ( !tmp_fn ) {
		fprintf(stderr, "Unable to get the name of a filter "	\
				"temporary file\n");
		return;
	}

	struct stat statbuf;
	if ( stat( tmp_fn, &statbuf ) != 0 ) {
		free( tmp_fn );
		return;
	}

	switch ( outformat ) {
	case PDF_FMT:
		switch ( color_idx ) {
		case 0:
			pdf_color = PDFCOLOR_CYAN;
			break;
		case 1:
			pdf_color = PDFCOLOR_MAGENTA;
			break;
		case 2:
			pdf_color = PDFCOLOR_YELLOW;
			break;
		case 3:
			pdf_color = PDFCOLOR_BLACK;
			break;
		}
		if ( outctx->pdfctx ) {
			if ( strcmp( fsuf, "ct" ) == 0 ) {
				pdf_add_tonemap( outctx->pdfctx, tmp_fn, pdf_color );
			} else if ( strcmp( fsuf, "m" ) == 0 ) {
				pdf_add_bitmap( outctx->pdfctx, tmp_fn, PDFCOLOR_WHITE );
			} else if ( strcmp( fsuf, "s" ) == 0 ) {
				pdf_add_bitmap( outctx->pdfctx, tmp_fn, pdf_color );
			} else {
				fprintf( stderr,
						 "Error: Unknown filter output class: %s\n",
						 fsuf );
			}
		}
		break;
	default:
		dump_file( outctx->output_file, tmp_fn );
	}
	
	free( tmp_fn );
}

/* Функция обработки файла изображения. */
int
process (char *file_name)
{

  /* Переменная для хранения идентификатора текущего процесса.
   * Используется для связи процессов фильтров и основного процесса. */
  pid_t pid;

  /* Файл-источник. */
  FILE *input_file = NULL;

  /* Параметры файла назначения. */
  struct output_ctx *outctx = NULL;
  
  /* Переменные для организации обмена информацией между
   * процессами.*/
  FILE *outpipe = NULL;		/* Канал для связи с фильтром. */

  /* Параметры фильтрации. */
  char f_cmd[MAXLINE];		/* Комманда для вызова фильтра. */
  int filter_count;		/* Количество фильтров. */

  /* Переменные для работы с многокрасочными изображениями. */
  int c0, c, cN;	/* Counter for colors */

  /* Переменные для работы с тегами TIFF файла. */
  /* Для хранения значений тегов. */
  uint16 phm;	/* TIFF TAG *PHOTOMETRIC* */
  tsample_t spp;	/* TIFF TAG *SAMPLESPERPIXEL* */
  uint16 r_unit;	/* TIFF TAG *RESOLUTIONUNIT* */
  uint16 tiff_planar;	/* Значение TIFF PlanarConfiguration */

  TIFF *tif = NULL;	/* Указатель на описание TIFF. */
  char *buf = NULL;     /* Буфер для хранения строки пикселей. */
  
  /* Парамеиры уменьшенной копии изображения. */
  char thumbnail_name[MAXLINE];	/* Имя файла уменьшенной копии. */
  TIFF *thumbnail = NULL;	/* Указатель на описание уменьшенной копии. */
  char *thumbnail_buf = NULL;	/* Буфер для хранения строки пикселей. */
  uint32 thumbnail_width;	/* Ширина уменьшенной копии. */
  uint32 thumbnail_height;	/* Высота уменьшенной копии. */
  


  /* Переменные для формирования уменьшенной копии. */
  double thumbnail_syf;		/* Вертикальная координата. */
  double thumbnail_step;	/* Вертикальный шаг. */
  int thumbnail_sy;		/* Номер строки в исходном изображении. */
  int thumbnail_y;		/* Номер строки в уменьшенной копии. */

  /* Параметры исходного изображения. */
  size_t ss;		/* Размер пикселя в байтах. */
  size_t rd;		/* Количество переданных байт. */
  int y;		/* Номер строки пикселов. */

  /* Переменные для вычисления степени прогресса обработки изображения. */
  int hd;		/* Счётчик десятых долей высоты изображения. */
  int yd, dc;		/* Счтчик десятых долей. */

  /* Счётчик */
  int i;

  /* Вспомогательная функция для освобождения занятой памяти
   * в случае аварийного завершения программы. */
  void cleanup() {
	/* Закрытие входного файла. */
	if (input_file != NULL && input_file != stdin)
		if (fclose(input_file) && want_verbose)
			fprintf(stderr, "Failed to close the input file\n");
  	/* Закрытие выходного файла. */
	if ( outctx ) {
		close_output( outctx );
		outctx = NULL;
	}
  	/* Закрытие коммуникационного канала. */
	if (outpipe != NULL)
		if (pclose(outpipe) && want_verbose)
			fprintf(stderr, "Filter error\n");
	/* Освобождение буфера изображения. */
	if (buf != NULL)
		_TIFFfree(buf);
	/* Закрытие файла TIFF. */
	if (tif != NULL)
		TIFFClose(tif);
	/* Освобождение буфера уменьшенной копии. */
	if (thumbnail_buf != NULL)
		_TIFFfree(thumbnail_buf);
	/* Закрытие файла TIFF уменьшенной копии. */
	if (thumbnail != NULL)
		TIFFClose(thumbnail);

	/* Удаление временных файлов, созданных фильтрами. */
	for (c = c0; c <= cN; c++) {
	  for (i = 0; i < filter_count; i++) {
		  delete_temporary_filter_file( "ct", pid, i, c );
		  delete_temporary_filter_file( "s", pid, i, c );
		  delete_temporary_filter_file( "m", pid, i, c );
	  }
	}
  }

  /* Инициализация очереди аварийной очистки. */
  init_cleanup(NULL);
  push_cleanup(cleanup);

  /* Подготовка к обработке изображения. Открытие файлов.
   * Настройка коммуникационных каналов между процессами. */

  /* Печать имени и версии программы перед началом работы
   * (режим повышенной информативности). */
  if (want_verbose)
	  fprintf(stderr, "Engrave "VERSION"\n");

  /* Проверка на указание параметров изображения в том случае,
   * если выбран режим обработки неформатированных данных. */
  if (is_raw) {
	  if (!width || !height || !hres || !vres) {
		  fprintf(stderr, "It is necessary to specify WIDTH, HEIGHT, HRES & VRES to process a RAW image stream (file).\n");
		  exit(EXIT_FAILURE);
	  }
  }

  /* Если требуется обработать форматированное изображение
   * в формате TIFF, то необходимо открыть указанный файл
   * с форматированным изображением. */
  if (!is_raw) {
	  tif = open_tif_file(file_name, &phm, &spp, &r_unit, &tiff_planar);
  } else if (file_name != NULL && strlen(file_name) > 0) {
	  /* Иначе производится попытка открытия файла с неформатированными
	   * данными или стандартного входного потока, если имя файла не
	   * указано. */
	  input_file = fopen(file_name, "r");
  } else {
	  input_file = stdin;
  }

  /* Поготовка выходного файла. */
  if ( prepare_output( file_name, output_name, &outctx) != 0 )
  {
	  exit(EXIT_FAILURE);
  }

  /* Проверка, указан ли хотя бы один фильтр? */
  if (filter[0] == '\0') {
	  fprintf(stderr, "It is necessary to specify filter(s) to process image through.\n");
	  exit(EXIT_FAILURE);
  }
  
  /* Подготовка к фильтрации изображения. */
  
  /* Получение идентификатора текущего процесса. */
  pid = getpid();
  
  /* Разбор коммандных строк фильтров. */
  parse_filters(f_cmd, &filter_count, pid);

  /* Печать диагностической информации
   * (режим повышенной информативности). */
  if (want_verbose) {
	  if (is_cmyk)
		  fprintf(stderr, "Processing CMYK image:\n");
	  else
		  fprintf(stderr, "Processing grayscale image:\n");
	  fprintf(stderr, "Width: %u\nHeight: %u\nHRes: %.2f\nVRes: %.2f\n", width, height, hres, vres);
	  fprintf(stderr, "Filters: %s\n", f_cmd);
  }

  /* Запуск дочернего процесса и открытие коммуникационного
   * канала к нему. */
  outpipe = popen(f_cmd, "w");
  /* Печать сообщения об ошибке и выход в случае неудачи. */
  if (outpipe == NULL) {
	  fprintf(stderr, "Can't create pipe attached to a filter process\n");
	  exit(EXIT_FAILURE);
  }

  /* Установка параметров обработки пикселов
   * в соответсвие с параметрами изображения. */
  if (is_cmyk) {
	  ss = 4;	/* 4 байта на пиксель для 4 красочного изображения. */
	  c0 = 0;	/* 0 для номера первого красителя. */
	  cN = 3;	/* 3 для номера последнего красителя. */ 
  } else {
	  ss = 1;	/* 1 байт для тонового изображения. */
	  c0 = 3;	/* 3 для номера первого */
	  cN = 3;	/* и последнего красителя. */
  }

  /* Подготовка к обработке строк изображения. */

  /* Выделение памяти для хранения строк исходного изображения. */
  buf = _TIFFmalloc(ss*width);
  /* Печать сообщения об ошибке и выход в случае неудачи. */
  if (buf == NULL) {
	  fprintf(stderr, "Scanline buffer allocation failed\n");
	  exit(EXIT_FAILURE);
  }

  /* Открытие файла уменьшенной копии изображения на запись.
   * Обработка ошибок. */
  if (want_preview) {
  	  /* Подготовка файла изображения уменьшенной копии. */
	  prepare_preview(thumbnail_name, pid, phm, ss, tiff_planar, &thumbnail, &thumbnail_buf, &thumbnail_width, &thumbnail_height);

	  /* Определение разрешения изображения.
	   * Определение единицы измерения разрешения. */
	  thumbnail_step = height/(double)thumbnail_height;
	  thumbnail_syf = 0;
	  thumbnail_sy = 0;
	  thumbnail_y = 0;
  }

  /* Печать индикатора прогресса (режим повышенной информативности) */
  if (want_verbose)
	  fprintf(stderr, "Progress: ");

  /* Последовательное чтение строк исходного изображения.
   * Строка записывается в буфер, откуда посылается по
   * межпроцессному каналу на фильтрацию. */

  /* Вычисление количества строк в десятой части изображения. */
  hd = height/10;

  /* Инициация счётчиков. */
  yd = 0;
  dc = 0;
  for (y = 0; y < height; y++) {
	  
	  if (is_raw) {
	  	  /* Неформатированныне данные читаются из стандартного
		   * входного потока. */
		  rd = fread(buf, ss, width, stdin);
		  /* Проверка фактического количества прочитанных байт. */
		  if (rd < width) {
			  fprintf(stderr, "Image stream suddenly closed.\n");
			  exit(EXIT_FAILURE);
		  }
	  } else
	  	  /* Чтение данных строки изображения из TIFF файла. */
		  TIFFReadScanline(tif, buf, y, 0);

	/* Если текущая строка должна быть включена в уменьшенную копию,
	 * то производится её масштабирование и запись во временный файл
	 * уменьшенной копии. */
	  if (thumbnail != NULL && y == thumbnail_sy) {
		  thumbnail_syf += thumbnail_step;
		  thumbnail_sy = rint(thumbnail_syf);
		  get_thumbnail_line(buf, ss, width, thumbnail_buf, thumbnail_width);
		  TIFFWriteScanline(thumbnail, thumbnail_buf, thumbnail_y++, 0);
	  }

	  /* Запись данных строки изображения в коммуникационный
	   * канал. Если единицы измерения оптического параметра входных
	   * и выходных данных различаются, то указывается необходимость
	   * инверсии. */
	  rd = fwritesmp(buf, ss, width, outpipe, miniswhite && want_intensity || !miniswhite && want_density, NULL);
	  if (rd < width) { /* Обработка ошибочной ситуации. */
		  fprintf(stderr, "Failed to transfer scanline data further\n");
		  exit(EXIT_FAILURE);
	  }
	 
	  /* Вычисление степени прогресса. */
	  yd++;
	  if (yd > hd) {
		  dc++;
		  yd = 0;
		  if (want_verbose)
			  fprintf(stderr, "%i0%% ",  dc);
	  }
  }

  /* Закрытие коммуникационного канала после чтения всех строк. */
  if (outpipe != NULL) {
	  if (pclose(outpipe)) {
		  if (want_verbose)
			  fprintf(stderr, "Filter error\n");
		  outpipe = NULL;
		  exit(EXIT_FAILURE);
	  }
	  outpipe = NULL;
  }

  /* Печать индикатора 100% (режим повышенной информативности). */
  if (want_verbose)
	  fprintf(stderr, "100%\n");

  if ( outctx ) {
	  /* Обработка файлов, созданных в результате работы фильтров. */
	  if ( outformat == EPS_FMT ) {
	  /* Вывод заголовка PostScript-программы и библиотечных файлов. */
		  ps_header( file_name, outctx->output_file );
	  }
	  
	  /* Слои изображения, созданные каждым из фильтров объединяются вместе.
	   * Последовательно обрабатываются файлы, содержащие информацию для
	   * каждого из красителей. */
	  for (c = c0; c <= cN; c++) {
		  /* Если включён режим выбора отдельных красителей, то производится
		   * проверка номера текущего красителя.*/
		  if (c0 == 0 && (want_c || want_m || want_y || want_k)) {
			  /* Если  текущий краситель не был индивидуально выбран, то он
				 пропускается. */
			  if (c == 0 && !want_c) {
				  continue;
			  }
			  if (c == 1 && !want_m) {
				  continue;
			  }
			  if (c == 2 && !want_y) {
				  continue;
			  }
			  if (c == 3 && !want_k) {
				  continue;
			  }
		  }

		  if ( outformat == EPS_FMT ) {
			  fprintf( outctx->output_file,
					  "%% Painting '%s' image color\n",
					  get_cmykcolor_name(c) );
			  /* В программу записывается комманда для выбора режима
			   * работы с одним красителем. */
			  fprintf( outctx->output_file, "%s\n", get_cmyk_color(c) );
			  fprintf( outctx->output_file, "%% Painting CT images\n" );
		  }
		  
		  /* Включение тонового изображения от каждого из фильтров. */
		  for (i = 0; i < filter_count; i++) {
			  use_temporary_filter_file( outctx, "ct", pid, i, c );
		  }

		  if ( outformat == EPS_FMT ) {
			  fprintf( outctx->output_file, "%% Painting mask images\n" );
		  }
		  
		  /* Включение маскирующего изображения от каждого из фильтров. */
		  for (i = 0; i < filter_count; i++) {
			  use_temporary_filter_file( outctx, "m", pid, i, c );
		  }

		  if ( outformat == EPS_FMT ) {
			  fprintf( outctx->output_file, "%% Painting stroke images\n" );
		  }
		  
		  /* Включение рисующего изображения от каждого из фильтров. */
		  for (i = 0; i < filter_count; i++) {
			  use_temporary_filter_file( outctx, "s", pid, i, c );
		  }
	  }

	  if ( outctx && outformat == EPS_FMT ) {
		  /* Печать завершающей части PostScript программы. */
		  fprintf( outctx->output_file,
				   "end\n"
				   "grestore\t%% Restore previous graphic state\n"
				   "showpage\n"
				   "%%%%EOF\n");
	  }
  
  }

  /* Получение признака записи результата в стандартный выход. */
  int to_stdout = ( outctx && outctx->output_file == stdout);

  /* Выполнение процедуры освобождения занятых ресурсов. */
  (*pop_cleanup())();

  /* Если указан соответствующий признак, то производится добавление к
   * PostScript-файлу уменьшенной копии иображения в формате TIFF. */
  if (want_preview && !to_stdout ) {
	  if ( outformat == EPS_FMT ) {
		  /* Добавление уменьшенной копии к PostScript-файлу. */
		  add_preview(output_name, thumbnail_name);
	  }
  }

  return 0;
}
