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

/* ������� � ��������� ����� ��������� ��������� PostScript-���������. */
void write_header(FILE *stream) {

	fprintf(stream, "%% Filter: ct filter from "PACKAGE" "VERSION"\n"
			"%%%%LanguageLevel 2\n"
			"gsave\t%% Save graphics state\n"
			"1.0 setcolor\n"
			"%f %f scale\n"
			"%% Image operator:\n"
			"<<\n"
			"\t/ImageType 1\n"
			"\t/Width %u\n"
			"\t/Height %u\n"
			"\t/BitsPerComponent 8\n"
			"\t/Decode %s\n"
			"\t/ImageMatrix [ %u 0 0 -%u 0 %u ]\n"		
			"\t/DataSource currentfile /ASCII85Decode filter\n"
			">>\n"
			"%%%%BeginData\n"
			"image\n",
			(float) width/hres*72,
			(float) height/vres*72,
			width, height, miniswhite ? "[0 1]" : "[1 0]", width, height, height);

}

/* ������� � ��������� ����� ��������� ��������� PostScript-���������. */
void write_footer(FILE *stream) {

	fprintf(stream, "%%%%EndData\n"
			"grestore\t%% Restore previous graphic state\n");

}

/* ������� ��������� ������ ����������� � ���� ������ �������� ASCII base 85.
 * ��������� a ���������� ��������� ����������� � �������� �����. ������ ��
 * count ���ޣ��� �� ss ���� ���������� � ������ buf. ���� ������ �������
 * ������ flush, �� ���������� ������ ���������� ������������ � ��������
 * �����.
 */
void write_ASCII(struct ascii85 *a, char *buf, size_t ss, size_t count, int flush) {

	int x;

	/* �������, ���� ����� ������ ����� 0. */
	if (ss == 0 || count == 0)
		return;

	/* ���������������� �������� ���ޣ��� ��� �����������. */
	for (x = 0; x < ss*count; x += ss)
		ascii85_encode(a, buf[x]);
	
	/* ���� ������� ������ ����������, �������� ����� ����������� �
	 * �������� �����.
	 */
	if (flush)
		ascii85_flush(a);

}

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
  
  /* ����� �������� ���������� ��� ������������ ������ � 4 ���������
   * �����.
   */
  FILE *outfile[] = {
	  NULL,
	  NULL,
	  NULL,
	  NULL
  };
  
  /* ����� �� 4 ����� ��� �������� �ͣ� ��������� ������. */
  char filenames[4][MAXLINE] = { "", "", "", "" };
  
  /* ����� �� 4 �������� ��� �������� ���������� ��� ����������� ��������
   * ����������� � ���������� �������������.
   */
  struct ascii85 *ascii85_p[] = {
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
		  if (ascii85_p[i] != NULL)
			  destroy_ascii85(ascii85_p[i]);
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
	 /* �������� ����� � ������, ��������������� ������ ��������� ������. */
	 if ((outfile[c] = open_tmp_file("ct", c, filenames[c])) == NULL) {
		/* ����� � ��������� ������, ���� ���� �� ��� ������. */
		exit(EXIT_FAILURE);
	 }
	 /* ������ ���������. */
	 write_header(outfile[c]);
	 /* �������� � ������������� ���������� ��� �����������. */
	 ascii85_p[c] = new_ascii85(outfile[c]);
	 if (ascii85_p[c] == NULL) {
		 fprintf(stderr, "%s: Failed to allocate an ascii85 structure.\n", program_name);
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
		  fprintf(stderr, "%s: Line %i. Image stream suddenly closed\n", y, program_name);
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
		  write_ASCII(ascii85_p[c], buf+c-c0, ss, width, y == (height-1));
  }

  /* ������ ����������� ����� PostScript-����. */
  for (c = c0; c <= cN; c++) 
	  write_footer(outfile[c]);

  /* ��������� �������� ��������� ����������. */
  OK = 1;
  
  /* ������������ ������� ��������. */
  (*pop_cleanup())();

  /* ����� � ��������� ��������� ���������� ������. */
  exit (0);

}

