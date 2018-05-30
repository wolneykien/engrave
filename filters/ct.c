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
 * ������ ct ���������� PostScript-��� ��� ������������� ������� ������
 * ��������� �����������, ����������� �� ������������ �����. ������ ������ ��
 * �������� �����������.
*/

#include <stdio.h>
#include <sys/types.h>
#include "system.h"
#include "filter.h"
#include "misc.h"

/* ��� ��������, ���������� ��������� ���������� ���������. */
#define EXIT_FAILURE 1

/* ���� �������� ��� ������� ������� ���������� � �������� �������. */
enum {DUMMY_CODE=129
};

/* ������ ������ �� ����� ����������� ����������. */
struct option const long_options[] =
{
	{NULL, 0, NULL, 0}
};

/* ����� ��������� ������� �������. */
void
usage_header(FILE *out)
{
  printf (_("%s - \
Image filter for 'engrave' package. Produces PostScript code for each image color channel reading a RAW CT image data from stdin\n"), program_name);

}

/* �������� �������. */
int
main (int argc, char **argv)
{

  /* ������ ����� ������� ��������������� ���������. */
  int opt_r;

  /* ������ ���ޣ�� � ������. */
  size_t ss;
  
  /* ����� �� 4 ����� ��� �������� �ͣ� ��������� ������. */
  const char *filenames[4] = { NULL, NULL, NULL, NULL };

  /* ����� ������� ����������� ������. */
  struct filter_writer *filter_writer_p = get_selected_filter_writer();
  
  /* ����� �� 4 �������� ��� �������� ���������� ��� ����������� ��������
   * ����������� � ���������� �������������.
   */
  void *filter_writer_ctx[] = {
	  NULL,
	  NULL,
	  NULL,
	  NULL
  };

  /* ����� ��� �������� ����� ������ �����������. */
  char *buf = NULL;

  /* �ޣ���� ������� �������. */
  int c0, c, cN;

  /* ����������� �ޣ���� �����-������. */
  size_t rd;

  /* ����� ������ �����������. */
  int y;

  /* ������� ��������� ���������� ������ �������. */
  int OK = 0;

  /* ������� ��������� ������� ������� ��������. */
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

  /* ������ ������ �������. ��������� ���������� ���������� ������ */

  /* ��������� ����� ��������. */
  program_name = argv[0];

  /* ������������� ������� ��������� �������. */
  init_cleanup(program_name);
  push_cleanup(cleanup);

  /* ������ ��������� ��������� ������. */
  opt_r = decode_switches (argc, argv, EXIT_FAILURE, long_options, NULL, &usage_header, NULL);

  /* �������� ������� ���������� �����������. */
  if (!width || !height || !hres || !vres) {
	  fprintf(stderr, "%s: WIDTH, HEIGHT, HRES & VRES should be specified.\n", program_name);
	  /* ����� � ��������� ������, ���� ���� �������� �� ��� ���������. */
	  exit(EXIT_FAILURE);
  }
  
  /* ������������� �ޣ����� �������� ������� � ������� ���ޣ�� � ������������ �
   * �������� ������ �����������.
   */
  if (is_cmyk) {
	  ss = 4;	/* 4 ����� ��� CMYK */
	  c0 = 0;	/* 4 �����, � 0 */
	  cN = 3;	/* �� 3 */
  } else {
	  ss = 1;	/* 1 ���� ��� Grayscale */
	  c0 = 3;	/* 1 ���� � 0 */
	  cN = 3;	/* �� 0 */
  }

  /* ��������� ������ ��� �������� ������ �����������. */
  buf = calloc(ss, width);
  if (buf == NULL) {
	  fprintf(stderr, "%s: Scanline buffer allocation failed\n", program_name);
	  /* ����� � ��������� ������, ���� ����� �������� �� �������. */
	  exit(EXIT_FAILURE);
  }

  /* �������� ��������� PostScript-������ � ��������������� ��������
   * ��� ����������� ���������� � ���������� �������������.
   */
  for (c = c0; c <= cN; c++) {
	 /* ��������� ����� ���������� �����. */
	 if ((filenames[c] = get_tmp_filter_file_name( "ct", c )) == NULL) {
		/* ����� � ��������� ������, ���� ��� �� ��������. */
		fprintf( stderr, "%s: Can't get temp file name\n", program_name );
		exit(EXIT_FAILURE);
	 }
	 /* �������� � ������������� ���������� ��� �����������. */
	 filter_writer_ctx[c] = filter_writer_p->open_tonemap( filenames[c] );
	 if ( filter_writer_ctx[c] == NULL ) {
		 fprintf(stderr,
				 "%s: Failed to initialize the writer.\n",
				 program_name);
		 /* ���������� ��������� � ��������� ������, � ������ ���������
		  * ������������� ���������.
		  */
		 exit(EXIT_FAILURE);
	 }
  }

  /* ����������������� ������ ����� ����������� �� ������������ �����,
   * ����������� � ���������� ������������� ��������� �������� �� �������
   * ������ � ������ �������������� ���������� � PostScript-�����.
   */
  for (y = 0; y < height; y++) {
	  /* ������ ������ �����������. */
	  rd = fread(buf, ss, width, stdin);
	  /* �������� ���������� ����������� ���ޣ���. */
	  if (rd < width) {
		  fprintf(stderr, "%s: Line %i. Image stream suddenly closed\n", program_name, y);
		  /* ����� � ��������� ������, ���� ���� ��������� ������
		   * ���ޣ���, ��� ������ �����������.
		   */
		  exit(EXIT_FAILURE);
	  }

	  /* ����������� �������� �������. */
	  for (c = c0; c <= cN; c++)
		  /* ������ ������ � ���������� �������������. ��� ������
		   * ��������� ������ ��������������� ������� ������ ������
		   * ��� ����, ����� ����� ���� ����� �������� �����������
		   * ����� PostScript-����. 
		   */
		  filter_writer_p->write_toneline( filter_writer_ctx[c],
										   buf + c - c0, ss,
										   width );
  }

  /* ������ ����������� ����� PostScript-����. */
  for (c = c0; c <= cN; c++) {
	  filter_writer_p->close( filter_writer_ctx[c] );
	  filter_writer_ctx[c] = NULL;
  }

  /* ��������� �������� ��������� ����������. */
  OK = 1;
  
  /* ������������ ������� ��������. */
  (*pop_cleanup())();

  /* ����� � ��������� ��������� ���������� ������. */
  exit (0);

}

