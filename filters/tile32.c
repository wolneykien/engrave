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
 * Фильтр tile32 герерирует PostScrpit-код для отрисовки 32 различных по
 * геометрии тайлов для передачи штриховой части изображения, передаваемого на
 * стандартный вход.
*/

#include <stdio.h>
#include <sys/types.h>
#include <string.h>

#include "system.h"
#include "filter.h"
#include "tile32f.h"
#include "misc.h"

/* Код завершения, сигнализирующий об ошибке. */

#define EXIT_FAILURE 1

/* Коды возврата функции разбора параметров с длиными именами. */
enum {DUMMY_CODE=129
};

/* Парметры фильтрации */

/* Специальные параметры фильтрации: */
int want_half = 0;		/* обрабатывать только (левую)
				 * половину
				 * изображения; */
char *histfn;			/* имя файла для вывода гистограммы
				 * использования функций; */

/* Переменные для ведения статистики использования тайлов: */
int phist[TILE_COUNT];          /* гистограмма позитивных тайлов; */
int nhist[TILE_COUNT];          /* гистограмма негативных тайлов; */
int zerotile = 0;               /* количество стационарных
				 * участков. */

/* Строковые значения параметров алгоритма анализа. */

/* Порог сравнения отдельных отсчётов (0 - 255). */
char *FThr_str;

/* Порог сравнения суммарных значений отсчётов (0 - 3*255). */
char *FThr2_str;

/* Коэффициент корреляции для диагонально расположенных отсчётов. */
char *FDcor_str;

/* Минимальная площадь штриха. */
char *minarea_str;

/* Параметры отключения обработки одельных цветовых каналов. */
char *passthrough_str;
int passthrough[] = {0, 0, 0, 0};

/* Параметры выборочной генерации масок. */
char *select_mask_str;
int select_mask[] = {1, 1};

/* Определение параметров командной строки. */
static struct option const long_options[] =
{
	{"half", no_argument, &want_half, 1},
	{"hist", required_argument, NULL, 0},
	{"ignore-outtest", no_argument, &outtest, 0},
	{"minarea", required_argument, NULL, 0},
	{"value-thr", required_argument, NULL, 0},
	{"sum-thr", required_argument, NULL, 0},
	{"dia-corr", required_argument, NULL, 0},
	{"passthrough", required_argument, NULL, 0},
	{"select-mask", required_argument, NULL, 0},
	{NULL, 0, NULL, 0}
};

/* Указатели на переменные, соответствующие параметрам коммандной строки. */
char **option_vars[] =
{
	NULL, &histfn, NULL, &minarea_str,
	&FThr_str, &FThr2_str, &FDcor_str, &passthrough_str,
	&select_mask_str
};

/* Определение вспомогательных функций. */

/* Вывод заголовка PostScript-программы в указанный поток, с возможностью
 * указания признака изображения маски.
 */
void write_header(FILE *stream, int mask) {
  if (stream != NULL) {
	fprintf(stream, "%% Filter: tile32 filter from "PACKAGE" "VERSION"\n"
			"%%%%LanguageLevel 2\n"
			"gsave\t%% Save grafics state\n"
			"%s setcolor\n"
			"%% Drawing %s tiles:\n"
			"0 %f translate\n"
			"%f %f scale\n"
			"currentfile /ASCII85Decode filter\n"
			"%%%%BeginData\n"
			"drawtiles\n",
			mask ? "0.0" : "1.0", mask ? "negative" : "positive",
			(float) height/hres*72,
			(float) 72/hres,
			(float) 72/vres);
  }
}

/* Вывод завершающей части PostScript-программы в указанный поток. */
void write_footer(FILE *stream) {
  if (stream != NULL) {
	fprintf(stream, "%%%%EndData\n"
			"grestore\t%% Restore previous graphic state\n");
  }
}

/* Основная функция, выполнямая для адаптивного растрирования: анализ
 * изображения окном, разделение сигнала и формирование записей о штриховой
 * части изображения в виде тайлов. Строки изображения располагаются в
 * указанном наборе буферов, номера и относительные площади тайлов записываются
 * в указанный выходной буфер. Информация требуемого цветового канала
 * выделяется из отсчётов на основании указанного смещения. Строки изображения
 * состоят из отсчётов указанной длины и из указанного количества таких
 * отсчётов. Информация о тайлах (номера и площади) требует кодирования в
 * виде строки символов, для помещения в PostScript-программу. Позитивные и
 * негативные тайлы кодируются по отдельности, поскольку относятся к различным
 * частям программы. Для их кодирования используются две указанные структуры.
 * Для синхронизации счётчиков пустых участков и пустых строк между вызовами
 * функции, они хранятся в структуре по указанному адресу.
 */
void maketiles(unsigned char *buf[], unsigned char *outbuf, \
	       int c0, int c, size_t ss, size_t len, \
	       struct ascii85 *pos_ascii85_p, \
	       struct ascii85 *neg_ascii85_p, \
	       struct maketiles_info *mi) {

	/* Окно отсчётов. */
	t_window window;

	/* Счётчик */
	int x;

	/* Признак полярности. */
	int neg;

	/* Номер тайла. */
	int tile_index;

	/* Относительная площадь тайла. */
	unsigned char tile_area;

	/* Текущая пара счётчиков для пустых участков и пустых строк. */
	unsigned int z;
	unsigned int zl;

	/* Текущая структура для кодирования информация о тайлах. */
	struct ascii85 *ascii85_p;

	/* Ширина половины изображения. */
	size_t half_len = len/2;

	/* Смещение цветового канала */
	size_t c_offs = c - c0;

	/* Сброс счётчиков пустых участков для позитивного и негативного
	 * штриховых изображений.
	 */
	mi->pz = 0;
	mi->nz = 0;

	/* Инициализация указателей на отсчёты в окне с учётом размера
	 * отсчёта в байтах и смещения цветового канала.
	 *
	 * Для доступа к указателям, хранящимся в структуре window используются
	 * специалные макроопределения.
	 */
	pB1 = buf[0] + 2*ss + c_offs;
	pA = buf[1] + ss + c_offs;
	pB = buf[1] + 2*ss + c_offs;
	pC = buf[1] + 3*ss + c_offs;
	pD1 = buf[2] + c_offs;
	pD = buf[2] + ss + c_offs;
	pE = buf[2] + 2*ss + c_offs;
	pF = buf[2] + 3*ss + c_offs;
	pF1 = buf[2] + 4*ss + c_offs;
	pG = buf[3] + ss + c_offs;
	pH = buf[3] + 2*ss + c_offs;
	pI = buf[3] + 3*ss + c_offs;
	pH1 = buf[4] + 2*ss + c_offs;

	/* Коррекция позиции записи в выходном буфере с учётом смещения
	 * цветового канала
	 */
	outbuf += c_offs;

	/* Сканирование строк изображения окном отсчётов. */
	for (x = 0; x < len; x++) {
		/* Если применение адаптивного растрирования ограничено только
		 * половиной изображения, то после пересечения границы данные
		 * изображения сразу же направляются в выходной буфер.
		 * Альтернативным условием является признак пропуска
		 * цветового канала.
		 */
	        if ((want_half && x > half_len)
		    || passthrough[c]) {
			tile_index = 0;
			*outbuf = E;
		} else {
			/* Анализ окрестной области с вычислением номера тайла
			 * и его относительной площади, запись фонового значения
			 * тона в выходной буфер.
			 */
			get_tile(window, &neg, &tile_index, &tile_area, outbuf);
		}

		/* Смещение окна и позиции записи вправо. */
		pA += ss;
		pB += ss;
		pC += ss;
		pD += ss;
		pE += ss;
		pF += ss;
		pG += ss;
		pH += ss;
		pI += ss;
		pB1 += ss;
		pD1 += ss;
		pH1 += ss;
		pF1 += ss;
		outbuf += ss;

		/* В случае, если был идентифицирован штриховой элемент,
		 * производится запись информации о нём в PostScript-код.
		 */
		if (tile_index) {
			/* Обработка информации о тайле производится в
			 * зависимости от его полярности.
			 */
                        if (neg) {
				/* Регистрация номера тайла в гистограмме
				 * масок.
				 */
				nhist[tile_index-1]++;
				/* Установка значений текущих переменных
				 * для обработки негативных тайов. */
				ascii85_p = neg_ascii85_p;
				z = mi->nz;
				zl = mi->nzl;
			} else {
				/* Регистрация номера тайла в гистограмме
				 * штрихов.
				 */
				phist[tile_index-1]++;
				/* Установка значений текущих переменных
				 * для обработки позитивных тайов. */
				ascii85_p = pos_ascii85_p;
				z = mi->pz;
				zl = mi->pzl;
			}


			/* Перед записью номера и площади тайла, записывается
			 * информация о горизонтальных и вертикальных пробелах,
			 * в соответствие со значениями счётчиков.
			 */
			if (zl) {
				ascii85_encode(ascii85_p, 0xFF);
				ascii85_encode(ascii85_p, 0xFF);
				ascii85_encode(ascii85_p, (unsigned char)((zl >> 8) & 0xFF));
				ascii85_encode(ascii85_p, (unsigned char)(zl & 0xFF));
			}
			if (z) {
				/* Если тайлу предшествует более 3 пробелов, то
				 * для экономии места используется компактная
				 * координатная нотация. */
				if (z > 3) {
					ascii85_encode(ascii85_p, 0xFF);
					ascii85_encode(ascii85_p, (unsigned char)((z >> 8) & 0xFF));
					ascii85_encode(ascii85_p, (unsigned char)(z & 0xFF));
				} else {
					/* Иначе, записываются подряд три
					 * пробела. */
					while (z--) {
						ascii85_encode(ascii85_p, (unsigned char) 0);
					}
				}
			}
				
			/* Записываются номер и площадь тайла. */
			ascii85_encode(ascii85_p, (unsigned char)tile_index);			
			ascii85_encode(ascii85_p, tile_area);
				
			/* Обновление и сброс счётчиков стационарных
			 * участков. */
			if (neg) {
				mi->nz = 0;
				mi->nzl = 0;
				mi->pz++;
			} else {
				mi->pz = 0;
				mi->pzl = 0;
				mi->nz++;
			}
		} else {
			/* На стационарном участке изображения только 
			 * увеличиваем счётчики пробелов. */
			mi->nz++;
			mi->pz++;
                        zerotile++;
		}
	}


	/* Увеличиваем счётчики пустых (стационарных) строк. */
	mi->nzl++;
	mi->pzl++;

}

/* Вывод заголовка краткой справки. */
void
usage_header (FILE *out)
{
  fprintf (out, _("%s - \
Image filter for 'engrave' package. Produces PostScript code for the stroke part of each image color channel reading a RAW CT image data from stdin\n"), program_name);
  
}

/* Вывод краткой справки по специальным праметрам. */
void
usage_params(FILE *out)
{

  fprintf(out, _("\
  --half	       process only the left half of an image\n\
  --hist=FILE	       write tile histogram data to FILE\n\
  --ignore-outtest     do not test stroke to cross the centra area\n\
  --minarea=VALUE      minimal line stroke area (defines thickness),\n\
		       default is 1\n\
  --value-thr=VALUE    threshold (0 - 255) to compare values,\n\
		       default is 13 (5%)\n\
  --sum-thr=VALUE      threshold (0 - 3*255) to compare summary\n\
		       values, default is 38.4\n\
  --dia-corr=VALUE     correlation coefficient of the diagonal and\n\
		       orthogonal sampling values\n\
  --passthrough=CMYK   ignores selected color channels\n\
  --select-mask=BW     select black, white or both correction\n\
                       images\n\
"));

}

/* Установка параметров алгоритма в соответствие с параметрами коммандной
 * строки. */
void
set_paramenetrs()
{

	/* Вспомогательный указатель. */
	char *endptr;

	/* Каждое из указанных значений необходимо преобразовать в числовую
	 * форму. */

	if (FThr_str != NULL) {
		FThr = (char) strtol(FThr_str, &endptr, 0);
		if (endptr == FThr_str) {
			fprintf(stderr, "Value comparison threshold should be an integer number.\n");
	  		/* Выход с признаком ошибки, если попытка преобразовани
	   	 	 * завершилась неудачей. */
	  		exit(EXIT_FAILURE);
		}
	}

	if (FThr2_str != NULL) {
		FThr = strtod(FThr2_str, &endptr);
		if (endptr == FThr2_str) {
			fprintf(stderr, "Summary values comparison threshold should be a decimal number.\n");
	  		/* Выход с признаком ошибки, если попытка преобразовани
	   	 	 * завершилась неудачей. */
	  		exit(EXIT_FAILURE);
		}
	}

	if (FDcor_str != NULL) {
		FThr = strtod(FDcor_str, &endptr);
		if (endptr == FDcor_str) {
			fprintf(stderr, "Diagonal correlator should be a decimal number.\n");
	  		/* Выход с признаком ошибки, если попытка преобразовани
	   	 	 * завершилась неудачей. */
	  		exit(EXIT_FAILURE);
		}
	}

	if (minarea_str != NULL) {
		minarea = (char) strtol(minarea_str, &endptr, 0);
		if (endptr == minarea_str) {
			fprintf(stderr, "Minimal line area should be a decimal number.\n");
	  		/* Выход с признаком ошибки, если попытка преобразовани
	   	 	 * завершилась неудачей. */
	  		exit(EXIT_FAILURE);
		}
	}

	if (passthrough_str != NULL) {
	  if (strchr(passthrough_str, 'C') != NULL) {
	    passthrough[0] = 1;
	  }
	  if (strchr(passthrough_str, 'M') != NULL) {
	    passthrough[1] = 1;
	  }
	  if (strchr(passthrough_str, 'Y') != NULL) {
	    passthrough[2] = 1;
	  }
	  if (strchr(passthrough_str, 'K') != NULL) {
	    passthrough[3] = 1;
	  }
	}
	if (select_mask_str != NULL) {
	  if (strchr(select_mask_str, 'B') != NULL) {
	    select_mask[0] = 1;
	  } else {
	    select_mask[0] = 0;
	  }
	  if (strchr(select_mask_str, 'W') != NULL) {
	    select_mask[1] = 1;
	  } else {
	    select_mask[1] = 0;
	  }
	}
}

/* Основная функция. */
int
main (int argc, char **argv)
{

  /* Номер первого необработанного аргумента. */
  int opt_r;

  /* Размер отсчёта в байтах. */
  size_t ss;
   /* Файл для записи гистограммы. */
  FILE *histf = NULL;

  /* Набор указателей на файлы для записи позитивных штриховых изображений. */
  FILE *pos_outfile[] = {
	  NULL,
	  NULL,
	  NULL,
	  NULL
  };

  /* Имена файлов для записи позитивных штриховых изображений. */
  char pos_filenames[4][MAXLINE] = { "\0", "\0", "\0", "\0" };
  
  /* Структуры используемые при коировании информации о позитивных тайлах. */
  struct ascii85 *pos_ascii85_p[] = {
	  NULL,
	  NULL,
	  NULL,
	  NULL
  };
  
  /* Набор указателей на файлы для записи маскирующих изображений. */
  FILE *neg_outfile[] = {
	  NULL,
	  NULL,
	  NULL,
	  NULL
  };
  
  /* Имена файлов для записи маскирующих изображений. */
  char neg_filenames[4][MAXLINE] = { "\0", "\0", "\0", "\0" };

  /* Структуры используемые при коировании информации о маскирующих тайлах. */
  struct ascii85 *neg_ascii85_p[] = {
	  NULL,
	  NULL,
	  NULL,
	  NULL
  };

  /* Набор буферов для хранения 5 строк изображения, которые затем
   * последовательно сканируются окном.
   */
  unsigned char *buf[] = {
	  NULL,
	  NULL,
	  NULL,
	  NULL,
	  NULL
  };

  /* Структура для синхронизации значений счётчиков между вызовами функции
   * генерации тайлов.
   */
  struct maketiles_info mi[4];
  
  /* Вспомогательная переменная для копирования указателей. */
  unsigned char *tmpbuf = NULL;
  
  /* Буфер для хранения фоновых значений тона, отправляемых на дальнейшую
   * обработку.
   */
  unsigned char *outbuf = NULL;	/* Buffer for output image data */ 
  
  /* Счётчик для цветовых каналов. */
  int c0, c, cN;

  /* Контрольный счётчик для опнраций ввода вывода. */
  size_t rd;

  /* Номер строки изображения. */
  int y;

  /* Счётчик для разнообразных операций. */
  int i;

  /* Признак успешного завершения обработки. */
  int OK = 0;

  /* Функция очистки занятых ресурсов. Освобождение занятой памяти и удаление
   * временных файлов, если обработка завершилась неудачей. В противном случае,
   * временные файлы удаляет основная программа, после их включения в основной
   * PostScript-файл.
   */
  void cleanup() {
	  int i;

	  if (!OK)
		  fprintf(stderr, "%s: Finished with error.\n", program_name);
	  for (i = 0; i < 4; i++) {
		  if (pos_ascii85_p[i] != NULL)
			  destroy_ascii85(pos_ascii85_p[i]);
		  if (pos_outfile[i] != NULL)
			  fclose(pos_outfile[i]);
		  if (!OK && pos_filenames[i][0] != '\0') {
			  fprintf(stderr, "%s: Delete temporary file: %s\n", program_name, pos_filenames[i]);
			  unlink(pos_filenames[i]);
		  }
		  if (neg_ascii85_p[i] != NULL)
			  destroy_ascii85(neg_ascii85_p[i]);
		  if (neg_outfile[i] != NULL)
			  fclose(neg_outfile[i]);
		  if (!OK && neg_filenames[i][0] != '\0') {
			  fprintf(stderr, "%s: Delete temporary file: %s\n", program_name, neg_filenames[i]);
			  unlink(neg_filenames[i]);
		  }
	  }
	  
	  for (i = 0; i < 5; i++)
		  if (buf[i] != NULL)
			  free(buf[i]);
	  if (outbuf != NULL)
		  free(outbuf);
  }

  /* Получение имени комманды. */
  program_name = argv[0];

  /* Инициализация функции аварийной очистки. */
  init_cleanup(program_name);
  push_cleanup(cleanup);

  /* Разбор аргуентов командной строки. */
  opt_r = decode_switches (argc, argv, EXIT_FAILURE, long_options, option_vars, &usage_header, &usage_params);

  /* Преобразование строковых значений параметров. */
  set_paramenetrs();

  /* Проверка указания всех необходимых параметров изображения. */
  if (!width || !height || !hres || !vres) {
	  fprintf(stderr, "%s: WIDTH, HEIGHT, HRES & VRES should be specified.\n", program_name);
	  /* Выход с признаком ошибки, если не все параметры изображения
	   * были указаны.
	   */
	  exit(EXIT_FAILURE);
  }
	 
  /* Установка размера отсчёта и счётчиков каналов в соответствие с указанным
   * типом изображения.
   */
  if (is_cmyk) {
	  ss = 4;	/* 4 байта для 4-красочного изображения; */
	  c0 = 0;	/* 4 цвета, с 0 */
	  cN = 3;	/* по 3 */
  } else {
	  ss = 1;	/* 1 байт для изображения в серых тонах; */
	  c0 = 3;	/* 1 цвет, с 0 */
	  cN = 3;	/* по 0 */
  }

  /* Выделение памяти под буфера. Дополнительные 2 отсчёта с каждой стороны
   * строки используются с целью уменьшения краевых эффектов. */
  for (i = 0; i < 5; i++) {
	  buf[i] = calloc(ss, width + 4);
	  if (buf[i] == NULL) {
		  fprintf(stderr, "%s: Scanline buffer #%i allocation failed\n", program_name, i);
		  /* Выход с признаком ошибки, если попытка выделения памяти,
		   * завершилась неудачей. */
		  exit(EXIT_FAILURE);
	  }
  }

  /* Выделение памяти под выходной буфер. */
  outbuf = calloc(ss, width);
  if (outbuf == NULL) {
	  fprintf(stderr, "%s: Output buffer allocation failed\n", program_name);
	  /* Выход с признаком ошибки, если попытка выделения памяти,
	   * завершилась неудачей. */
	  exit(EXIT_FAILURE);
  }

  /* Инициализация гистограм. */
  for (i = 0; i < TILE_COUNT-1; i++) {
	  phist[i] = 0;
	  nhist[i] = 0;
  }

  /* Открытие временных файлов и инициализация кодировщиков. */
  for (c = c0; c <= cN; c++) {
	 /* Открытие файлов. */
    if (select_mask[0]) {
      pos_outfile[c] = open_tmp_file("s", c, pos_filenames[c]);
    }
    if (select_mask[1]) {
      neg_outfile[c] = open_tmp_file("m", c, neg_filenames[c]);
    }
	 /* Вывод заголовков. */
	 write_header(pos_outfile[c], 0);
	 write_header(neg_outfile[c], 1);
	 
	 /* Инициализация кодировщиков. */
	 pos_ascii85_p[c] = new_ascii85(pos_outfile[c]);
	 neg_ascii85_p[c] = new_ascii85(neg_outfile[c]);
	 if (pos_ascii85_p[c] == NULL || neg_ascii85_p[c] == NULL) {
		 fprintf(stderr, "%s: Failed to allocate an ascii85 structure.\n", program_name);
		 /* Выход с признаком ошибки, если инициализация завершилась
		  * ошибкой.
		  */
		 exit(EXIT_FAILURE);
	 }
  }

  /* Вывод инфрмации о текущих настройках. */
  if (want_verbose) {
    if (histfn != NULL) {
      fprintf(stderr, "[%s] Histogram file: %s\n", program_name, histfn);
    }
    fprintf(stderr, "[%s] Byte threshold: %u\n", program_name, FThr);
    fprintf(stderr, "[%s] Summ threshold: %.2f\n", program_name, FThr2);
    fprintf(stderr, "[%s] Diagonal correlator: %.2f\n", program_name, FDcor);
    fprintf(stderr, "[%s] Minimum line area: %u\n", program_name, minarea);
    fprintf(stderr, "[%s] Middle area test: %s\n", program_name, outtest ? "on" : "off");
    if (passthrough_str != NULL) {
      fprintf(stderr, "[%s] Passthrough separations: %s\n", program_name, passthrough_str);
    }
    if (select_mask_str != NULL) {
      fprintf(stderr, "[%s] Selected correction images: %s\n", program_name, select_mask_str);
    }
  }

  /* Чтение и обработка строк изображения. */

  /* Предварительное чтение строки изображения в буфер 4-ой строки (3), в
   * позицию 3-го отсчёта (2). */
  rd = freadsmp(buf[3]+2*ss, ss, width, stdin, miniswhite);
  /* Если количество прочитанных отсчётов оказалось меньше длины строки
   * изображения, то производится выход с признаком ошибки. */
  if (rd < width) {
	  fprintf(stderr, "%s: Line 0. Image stream suddenly closed (%i samples has been read)\n", program_name, rd);
	  exit(EXIT_FAILURE);
  }
  /* Копирование крайних отсчётов 4-ой строки (3) для минимизации краевых
   * эффектов. */
  edgecpy(buf[3], width, ss);
 
  /* Если в изображении больше одной строки, то производится предварительное
   * чтение строки изображения в буфер 5-ой строки (4), в позицию 3-го отсчёта
   * (2). */
  if (height > 1) {
	  rd = freadsmp(buf[4]+2*ss, ss, width, stdin, miniswhite);
	  /* Если количество прочитанных отсчётов оказалось меньше длины строки
	   * изображения, то производится выход с признаком ошибки. */
	  if (rd < width) {
		  fprintf(stderr, "%s: Line 1. Image stream suddenly closed (%i samples has been read)\n", program_name, rd);
		  exit(EXIT_FAILURE);
	  } 
  }
  /* Копирование крайних отсчётов 5-ой строки (4) для минимизации краевых
   * эффектов. */
  edgecpy(buf[4], width, ss);

  /* Копирование 4-ой строки (3) дважды: в 3-ю (2) и вторую (1) строку с целью
   * уменьшения краевых эффектов. */
  memcpy(buf[2], buf[3], ss*(width+4));
  memcpy(buf[1], buf[2], ss*(width+4));

  /* Инициализация счётчиков для функции генерации тайлов. */
  for (c = c0; c <= cN; c++)
	  init_maketiles_info(&mi[c]);
  
  /* Главный цикл обработки изображения. Поскольку 2 строки уже было
   * предварительно прочитано, начинаем обработку с 3-ей строки. */
  for (y = 2; y < height; y++) {
	  /* Циклический перенос указателей на одну строку вверх, после чего
	   * в буфере 5-ой строки (4) находится старое содержание первой
	   * строки (0), которое затем заменяется новой строкой, получаемой со
	   * стандартного входа. */
	  tmpbuf = buf[0];
	  buf[0] = buf[1];
	  buf[1] = buf[2];
	  buf[2] = buf[3];
	  buf[3] = buf[4];
	  buf[4] = tmpbuf;

	  rd = freadsmp(buf[4]+2*ss, ss, width, stdin, miniswhite);

	  /* Контроль количества прочитанных отсчётов и выход с признаком
	   * ошибки, если было прочитано меньше длины строки. */
	  if (rd < width) {
		  fprintf(stderr, "%s: Line %i. Image stream suddenly closed (%i samples has been read)\n", program_name, y, rd);
		  exit(EXIT_FAILURE);
	  }

	  /* Копирование крайних отсчётов для минимизации краевых эффектов. */
	  edgecpy(buf[4], width, ss);	/* Copy edges of the new line */

	  /* Последовательная обработка цветовых каналов с генерацией тайлов. */
	  for (c = c0; c <= cN; c++) {
		  maketiles(buf, outbuf, c0, c, ss, width, pos_ascii85_p[c], neg_ascii85_p[c], &mi[c]);
	  }

	  /* Отправка строки изображения с фоновыми значениями тона на
	   * дольнейшую обработку. */
	  write_outbuf(outbuf, ss, width);
  }
  
  /* Если в изображении была всего одна строка, она копируется ниже. */
  if (height == 1)
	  memcpy(buf[4], buf[3], ss*(width+4));

  /* Обработка одной или двух последних строк изображения. */
  for (i = 0; i < (height > 1 ? 2 : 1); i++) {
	  /* Циклический сдвиг указателей. */
	  tmpbuf = buf[0];
	  buf[0] = buf[1];
	  buf[1] = buf[2];
	  buf[2] = buf[3];
	  buf[3] = buf[4];
	  buf[4] = tmpbuf;

	  /* Копирование последней строки ниже, для минимизации краевых
	   * эффектов. */
	  memcpy(buf[4], buf[3], ss*(width+4));
	  
	  /* Последовательная обработка цветовых каналов с генерацией тайлов. */
	  for (c = c0; c <= cN; c++) {
		  maketiles(buf, outbuf, c0, c, ss, width, pos_ascii85_p[c], neg_ascii85_p[c], &mi[c]);
	  }
	  /* Отправка строки изображения с фоновыми значениями тона на
	   * дольнейшую обработку. */
	  write_outbuf(outbuf, ss, width);
  }
  
  /* Сброс буферов кодировщиков для каждого цветового канала в соответствующие
   * выходные потоки. Вывод завершающих частей фрагментов PostScript-программы.
   */
  for (c = c0; c <= cN; c++) {
	  /* Сброс буферов. */
	  ascii85_encode(pos_ascii85_p[c], 0xFF);
	  ascii85_encode(pos_ascii85_p[c], 0xFF);
	  ascii85_encode(pos_ascii85_p[c], 0xFF);
	  ascii85_flush(pos_ascii85_p[c]);
	  ascii85_encode(neg_ascii85_p[c], 0xFF);
	  ascii85_encode(neg_ascii85_p[c], 0xFF);
	  ascii85_encode(neg_ascii85_p[c], 0xFF);
	  ascii85_flush(neg_ascii85_p[c]);

	  /* Вывод завершающих частей. */
	  write_footer(pos_outfile[c]);
	  write_footer(neg_outfile[c]);
  }

  /* Производится вывод гистограммы, если было указано имя файла для записи. */
  if (histfn != NULL) {
	  histf = fopen(histfn, "w");
	  if (histf != NULL) {
		  fprintf(histf, "#0: %i\n", zerotile);
		  for (i = 0; i < TILE_COUNT; i++)
			  fprintf(histf, "#%i: %i\n", i+1, phist[i]);
		  for (i = 0; i < TILE_COUNT; i++)
			  fprintf(histf, "#-%i: %i\n", i+1, nhist[i]);
		  fclose(histf);
	  } else {
		  /* Вывод сообщения об ошибке без завершения работы, если
		   * попытка записи завершилась ошибкой. */
		  fprintf(stderr, "Writing histogram to file %s failedi.\n", histfn);
	  }
  }

  /* Установка признака удачного завершения. */
  OK = 1;
  
  /* Освобождение занятых ресурсов. */
  (*pop_cleanup())();
  
  /* Выход с признаком успешного завершения. */
  exit (0);
  
}
