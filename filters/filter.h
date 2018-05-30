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


#ifndef __FILTER_H
#define __FILTER_H

/* ���� � ���������, �������� ����������� �������, ������� ������������ ���
 * ���������� ��������.
 */

#include <getopt.h>

/* ��� �������������� ��������. */
extern char *program_name;

/* ����� ������������ �������. */
extern pid_t pid;

/* ���������� ����� �������. */
extern int fidx;

/* ������� ������ ���������� ���������������. */
extern int want_verbose;

/* �������� ���������� */
/* ��������� �����������: */
extern unsigned long int width;		/* ������ �����������; */
extern int unsigned long height;        /* ������ �����������; */
extern float hres;                      /* ���������� �� �����������; */
extern float vres;			/* ���������� �� ���������; */
extern int is_cmyk;			/* ������� 4-���������� �����������; */
extern int miniswhite;			/* ������� ����������� �����������. */

typedef enum { FILTER_EPS_FMT, FILTER_TIFF_FMT } filter_outformat_t;
extern filter_outformat_t filter_outformat;

/* ��� �������, ���������� ��������� ������� �������. */
typedef void(*usage_header_f)(FILE *out);

/* ��� �������, ���������� ������� ������� �� ����������� ����������
 *  �������.
 */
typedef void(*usage_params_f)(FILE *out);

const char *get_tmp_filter_file_name( const char *fsuf, int color_idx );

/**
 * ��������� � ��������� ����������� �������.
 */
struct filter_writer {
	/**
	 * �������������� ���������� ��� ������ ����� ������
	 * � ���� � ������ #outfile. ��������� ������� #mask �����������
	 * ����������� (�����). ���������� ��������� �� ��������,
	 * ������� ����� ������������ � ������ ��������.
	 */
	void * (*open_tilemap)      ( const char *outfile, int mask );
	
	/**
	 * �������������� ���������� ��� ������ �������� �����������
	 * � ���� � ������ #outfile. ���������� ��������� �� ��������,
	 * ������� ����� ������������ � ������ ��������.
	 */
	void * (*open_tonemap)      ( const char *outfile );

	/**
	 * �������� #zl ����� ������. ���� #zl > 1, �� �������������
     * ������������ ������ ������.
	 */
	void   (*write_tile_lines) ( void *ctx, unsigned int zl );

	/**
	 * �������� #z ��������.
	 */
	void   (*write_spaces)      ( void *ctx, unsigned int z );

	/**
	 * �������� ���� � ������� #tile_index � �������������
	 * �������� #tile_area.
	 */
	void   (*write_tile)        ( void *ctx,
								  unsigned char tile_index,
								  unsigned char tile_area );

	/**
	 * ������� ������ �������� �����������. ������ �� #count ���ޣ���
	 * �� #ss ���� ���������� � ������ #buf.
	 */
	void  (*write_toneline)     ( void *ctx, const char *buf,
								  size_t ss, size_t count );

	/**
	 * ��������� ���������� � ����������� �������.
	 */
	void   (*close)             ( void *ctx );
};

char *get_filter_option(int i, char *dst_fopt, char *fopts);

int decode_switches (int argc, char **argv, int error_code, \
		     struct option const *long_options, \
		     char ***option_vars, usage_header_f usage_header, \
		     usage_params_f usage_params);

void write_outbuf(char *outbuf, size_t ss, size_t len);

/**
 * ���������� ��������� �� ��������� ����������.
 * ������ ���������� ����� ������� �����.
 */
struct filter_writer *get_selected_filter_writer ();

#endif /* __FILTER_H */
