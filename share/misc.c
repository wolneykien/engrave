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


/* ���������� ��������������� �������. */

#include <stdio.h>
#include <sys/types.h>
#include "system.h"
#include "misc.h"

/* ����� ������� PostScript ��� ��������� �������� �����������, ���������������
 * �������������� ����������.
 */
char *cmykcolors[] = {
	CYAN_COLOR,
	MAGENTA_COLOR,
	YELLOW_COLOR,
	BLACK_COLOR
};

/* ����� ���������, ������������ ��� ����������� ������� �� ����ң� ������
 * ��� ����������� �ͣ� ������.
 */
char *color_suf[] = {
	CYAN_SUF,
	MAGENTA_SUF,
	YELLOW_SUF,
	BLACK_SUF
};

/* ����� �ͣ� ��� ���������� ������� �� ����ң� ������. */
char *cmykcolor_name[] = {
	CYAN_COLOR_NAME,
	MAGENTA_COLOR_NAME,
	YELLOW_COLOR_NAME,
	BLACK_COLOR_NAME
};

/* ��� ��������. */
char *prg_name = NULL;


/* ��������� ������ ��� ����������� �������� ������ (�����) �������
 * ��������� �������. */
struct cleanup_qs {
	struct cleanup_qs *next;
	cleanup_f cleanup;
};

/* ��������� �� ������� �����. */
struct cleanup_qs *queue = NULL;

/* ������������� �ͣ� ������ ����� '/' ��� ����������� ����. */

void pathcat(char *dest, const char *src) {
	if (dest[strlen(dest)-1] != '/' && src[0] != '/')
		strcat(dest, "/");
	if(dest[strlen(dest)-1] == '/' && src[0] == '/')
		dest[strlen(dest)-1] = '\0';
	strcat(dest, src);
}

/* �������� ����� � ����������� �������� � ����� �����. */
void delete_ext(char *filename) {

	size_t i;

	i = strlen(filename);
	while (i > 0 && filename[i] != '.')
		i--;
	if (i > 0)
		filename[i] = '\0';

}

/* �������� ��������� ������� ������� �� ������� �����. */
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

/* ������ ������� ������� � ������� �����. */
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

/* ���������������� ���������� ����� ����� �������. */
void do_cleanup() {

	cleanup_f cleanup;

	cleanup = (cleanup_f) pop_cleanup();
	while (cleanup != NULL) {
		cleanup();
		cleanup = (cleanup_f) pop_cleanup();
	}

}

/* ������������� ��������� ��������� �������. */
void init_cleanup(char *program_name) {

	prg_name = program_name;

	queue = NULL;
	atexit(do_cleanup);

}

/* �������� ��� ���������� ����� (��������� ��� �� ���������� ������) ��� ������
 * ��������� PostScript-���������, ��������������� �������� �������, ������
 * ��������, ������ ������� � ������ ��������� ������.
 */
char *get_tmp_file_name(char *str, const char *fsuf, pid_t pid, int fidx, int color_idx) {

	/* ���������� ��� �������� ����� � ���� � �����. */
	char outpath[MAXLINE];
	char outname[MAXLINE];

	strcpy(outpath, P_tmpdir);
	snprintf(outname, sizeof(outname), "%u.%u.%s", pid, fidx, fsuf);
	pathcat(outpath, "/");
	sprintf(str, "%s%s.%s", outpath, outname, get_cmyk_color_suf(color_idx));

	return str;

}

/* �������� �������� ��� ��������� ��������� ������������ �� ������ ���������
 * ������.
 */
char *get_cmyk_color(int i) {
	if (i < 0 || i > 3)
		return (char *) NULL;
	return cmykcolors[i];
}

/* �������� ������� ����� ��������� ������ �� ��� ������. */
char *get_cmyk_color_suf(int i) {
	if (i < 0 || i > 3)
		return (char *) NULL;
	return color_suf[i];
}

/* �������� �������� ��������� ������ �� ��� ������. */
char *get_cmykcolor_name(int i) {
	if (i < 0 || i > 3)
		return (char *) NULL;
	return cmykcolor_name[i];
}

/* ��������� ��������� ����� � ������������ � ������� ������� ������. */
void set_cyan_index(int i) {

	if (i >= 0 && i < 4) {
		cmykcolors[i] = CYAN_COLOR;
		color_suf[i] = CYAN_SUF;
	}
}

/* ��������� ��������� ����� � ������������ � ������� ��������� ������. */
void set_magenta_index(int i) {

	if (i >= 0 && i < 4) {
		cmykcolors[i] = MAGENTA_COLOR;
		color_suf[i] = MAGENTA_SUF;
	}
}

/* ��������� ��������� ����� � ������������ � ������� ֣���� ������. */
void set_yellow_index(int i) {

	if (i >= 0 && i < 4) {
		cmykcolors[i] = YELLOW_COLOR;
		color_suf[i] = YELLOW_SUF;
	}
}

/* ��������� ��������� ����� � ������������ � ������� ޣ���� ������. */
void set_black_index(int i) {

	if (i >= 0 && i < 4) {
		cmykcolors[i] = BLACK_COLOR;
		color_suf[i] = BLACK_SUF;
	}
}

/* �������� ������ ����������� � ��������� ������, ��������� �� ���ޣ���
 * ��������� ����� � ���������� ���������� ����� ���ޣ���.
 */
void invertsmp(void *buf, size_t ss, size_t count) {

	unsigned int i;
	unsigned char *c;

	for (i = 0; i < ss*count; i++) {
		c = buf+i;
		*c = 255 - *c;
	}

}

/* ������ ������ ����������� � ��������� �����, ��������� �� ���ޣ���
 * ��������� ����� � ���������� ���������� ����� ���ޣ���, �� ����������
 * ������, � ������������ �������� �������� ������.
 */
size_t freadsmp(void *buf, size_t ss, size_t count, FILE *stream, int neg) {

	size_t rd;

	rd = fread(buf, ss, count, stream);
	if (neg)
		invertsmp(buf, ss, count);

	return rd;

}

/* ������ ������ ����������� �� ���������� ������, ��������� �� ���ޣ���
 * ��������� ����� � ���������� ���������� ����� ���ޣ���, � ���������
 * �����, � ������������ �������� ������������ ������.
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

