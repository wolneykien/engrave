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


#ifndef __PDFWRITER_H
#define __PDFWRITER_H

#ifdef __cplusplus
extern "C" {
#endif

/* ���������� ��� ������ ������������ ����������� � ������� PDF. */

/**
 * ��������� ����� ��� ������� � ����.
 */
typedef enum {
	PDFCOLOR_BLACK,
	PDFCOLOR_CYAN,
	PDFCOLOR_MAGENTA,
	PDFCOLOR_YELLOW,
	PDFCOLOR_WHITE
} pdfcolor_t;

/**
 * ��������� PDF ���� #filename ��� ������. ������� �����������
 * ���������� � #width � #height.
 * ���������� ��������� �� �������� ��� #NULL � ������ ������.
 */
void * pdf_open_file( const char *filename, double width, double height );

/**
 * ��������� � PDF #ctx �������������� ���� �� ����� #tifffile.
 * �������� #color ���������� ���� �������.
 * ���������� 0 � ������ ������, � ��-0 � ������ ������.
 */
int pdf_add_bitmap( void *ctx, const char *tifffile, pdfcolor_t color );

/**
 * ��������� � PDF #ctx ������� ���� �� ����� #tifffile.
 * �������� #color ���������� ���� ��������.
 * ���������� 0 � ������ ������, � ��-0 � ������ ������.
 */
int pdf_add_tonemap( void *ctx, const char *tifffile, pdfcolor_t color );

/**
 * ��������� �������� � ���������� PDF ����.
 */
int pdf_close( void *ctx );

#ifdef __cplusplus
}
#endif
	
#endif /* __PDFWRITER_H */
