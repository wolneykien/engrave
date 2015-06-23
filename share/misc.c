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

#include <stdio.h>
#include <sys/types.h>
#include "system.h"
#include "misc.h"

/* Набор комманд PostScript для установки цветовых пространств, соответствующих
 * индивидуальным красителям.
 */
char *cmykcolors[] = {
	CYAN_COLOR,
	MAGENTA_COLOR,
	YELLOW_COLOR,
	BLACK_COLOR
};

/* Набор суффиксов, используемых для обозначения каждого из четырёх цветов
 * при составлении имён файлов.
 */
char *color_suf[] = {
	CYAN_SUF,
	MAGENTA_SUF,
	YELLOW_SUF,
	BLACK_SUF
};

/* Набор имён для обзначения каждого из четырёх цветов. */
char *cmykcolor_name[] = {
	CYAN_COLOR_NAME,
	MAGENTA_COLOR_NAME,
	YELLOW_COLOR_NAME,
	BLACK_COLOR_NAME
};

/* Имя комманды. */
char *prg_name = NULL;


/* Структура данных для образования связного списка (стека) функций
 * аварийной очистки. */
struct cleanup_qs {
	struct cleanup_qs *next;
	cleanup_f cleanup;
};

/* Указатель на вершину стека. */
struct cleanup_qs *queue = NULL;

/* Присоединение имён файлов через '/' для образование пути. */

void pathcat(char *dest, const char *src) {
	if (dest[strlen(dest)-1] != '/' && src[0] != '/')
		strcat(dest, "/");
	if(dest[strlen(dest)-1] == '/' && src[0] == '/')
		dest[strlen(dest)-1] = '\0';
	strcat(dest, src);
}

/* Удаление точки и последующих символов в конце файла. */
void delete_ext(char *filename) {

	size_t i;

	i = strlen(filename);
	while (i > 0 && filename[i] != '.')
		i--;
	if (i > 0)
		filename[i] = '\0';

}

/* Добавить указанную функцию очистки на вершину стека. */
int push_cleanup(cleanup_f cleanup) {

	struct cleanup_qs *q, *c;

	c = (struct cleanup_qs*) malloc(sizeof(struct cleanup_qs));
	if (c == NULL)
		return 1;
	c->next = NULL;
	c->cleanup = cleanup;

	if (queue != NULL) {
		q = queue;
		while (q->next != NULL)
			q = q->next;
		q->next = c;
	} else
		queue = c;

	return 0;

}

/* Снятие функции очистки с вершины стека. */
cleanup_f pop_cleanup() {

	struct cleanup_qs *q;
	cleanup_f cleanup;

	if (queue != NULL) {
		if (queue->next != NULL) {
			q = queue;
			while (q->next->next != NULL)
				q = q->next;
			cleanup = q->next->cleanup;
			free(q->next);
			q->next = NULL;
		} else {
			cleanup = queue->cleanup;
			free(queue);
			queue = NULL;
		}
	} else
		cleanup = NULL;

	return cleanup;

}

/* Последовательное выполнение всего стека очистки. */
void do_cleanup() {

	cleanup_f cleanup;

	cleanup = (cleanup_f) pop_cleanup();
	while (cleanup != NULL) {
		cleanup();
		cleanup = (cleanup_f) pop_cleanup();
	}

}

/* Инициализация механизма аварийной очистки. */
void init_cleanup(char *program_name) {

	prg_name = program_name;

	queue = NULL;
	atexit(do_cleanup);

}

/* Получить имя временного файла (разместив его по указанному адресу) для записи
 * фрагмента PostScript-программы, соответствующей суффиксу фильтра, номеру
 * процесса, номеру фильтра и номеру цветового канала.
 */
char *get_tmp_file_name(char *str, const char *fsuf, pid_t pid, int fidx, int color_idx) {

	/* Переменные для хранения имени и пути к файлу. */
	char outpath[MAXLINE];
	char outname[MAXLINE];

	strcpy(outpath, P_tmpdir);
	snprintf(outname, sizeof(outname), "%u.%u.%s", pid, fidx, fsuf);
	pathcat(outpath, "/");
	sprintf(str, "%s%s.%s", outpath, outname, get_cmyk_color_suf(color_idx));

	return str;

}

/* Получить комманду для устуновки цветового пространства по номеру цветового
 * канала.
 */
char *get_cmyk_color(int i) {
	if (i < 0 || i > 3)
		return (char *) NULL;
	return cmykcolors[i];
}

/* Получить суффикс имени цветового канала по его номеру. */
char *get_cmyk_color_suf(int i) {
	if (i < 0 || i > 3)
		return (char *) NULL;
	return color_suf[i];
}

/* Получить название цветового канала по его номеру. */
char *get_cmykcolor_name(int i) {
	if (i < 0 || i > 3)
		return (char *) NULL;
	return cmykcolor_name[i];
}

/* Поставить указанный номер в соответстиве с каналом голубой краски. */
void set_cyan_index(int i) {

	if (i >= 0 && i < 4) {
		cmykcolors[i] = CYAN_COLOR;
		color_suf[i] = CYAN_SUF;
	}
}

/* Поставить указанный номер в соответстиве с каналом пурпурной краски. */
void set_magenta_index(int i) {

	if (i >= 0 && i < 4) {
		cmykcolors[i] = MAGENTA_COLOR;
		color_suf[i] = MAGENTA_SUF;
	}
}

/* Поставить указанный номер в соответстиве с каналом жёлтой краски. */
void set_yellow_index(int i) {

	if (i >= 0 && i < 4) {
		cmykcolors[i] = YELLOW_COLOR;
		color_suf[i] = YELLOW_SUF;
	}
}

/* Поставить указанный номер в соответстиве с каналом чёрной краски. */
void set_black_index(int i) {

	if (i >= 0 && i < 4) {
		cmykcolors[i] = BLACK_COLOR;
		color_suf[i] = BLACK_SUF;
	}
}

/* Инверсия строки изображения в указанном буфере, состоящей из отсчётов
 * указанной длины и указанного количества таких отсчётов.
 */
void invertsmp(void *buf, size_t ss, size_t count) {

	unsigned int i;
	unsigned char *c;

	for (i = 0; i < ss*count; i++) {
		c = buf+i;
		*c = 255 - *c;
	}

}

/* Чтение строки изображения в указанный буфер, состоящей из отсчётов
 * указанной длины и указанного количества таких отсчётов, из указанного
 * потока, с возможностью инверсии читаемых данных.
 */
size_t freadsmp(void *buf, size_t ss, size_t count, FILE *stream, int neg) {

	size_t rd;

	rd = fread(buf, ss, count, stream);
	if (neg)
		invertsmp(buf, ss, count);

	return rd;

}

/* Запись строки изображения из указанного буфера, состоящей из отсчётов
 * указанной длины и указанного количества таких отсчётов, в указанный
 * поток, с возможностью инверсии записываемых данных.
 */
size_t fwritesmp(void *buf, size_t ss, size_t count, FILE *stream, int neg, void *outbuf) {

	size_t rd;

	if (neg) {
		if (outbuf != NULL) {
			invertsmp(outbuf, ss, count);
			rd = fwrite(outbuf, ss, count, stream);
		} else {
			invertsmp(buf, ss, count);
			rd = fwrite(buf, ss, count, stream);
		}
	} else
		rd = fwrite(buf, ss, count, stream);

	return rd;

}

