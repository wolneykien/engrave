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


/* ���������� ��� ������ �������� ������ � ���� ������, � �������
ASCII-85. */

#include "ascii85.h"

void * ascii85_open( FILE *outfile );
void ascii85_write_empty_lines( void *ctx, unsigned int zl );
void ascii85_write_spaces( void *ctx, unsigned int z );
void ascii85_write_tile( void *ctx, unsigned char tile_index,
						 unsigned char tile_area );
void ascii85_close( void *ctx );

/**
 * ��������� � ��������� ����������� ASCII-85.
 */
struct filter_writer ascii85_filter_writer = {
	.open_tilemap      = ascii85_open_tilemap,
	.open_tonemap      = ascii85_open_tonemap,
	.write_empty_lines = ascii85_write_empty_lines,
	.write_spaces      = ascii85_write_spaces,
	.write_tile        = ascii85_write_tile,
	.write_toneline    = ascii85_write_toneline,
	.close             = ascii85_close
};

/**
 * ��������� ��� �������� ���������� ��� ����������� ���������
 * ����������� � ��������� �������������.
 */
struct ascii85 {
	int line_break;
	int offset;
	char buffer[4];
	char tuple[6];
	FILE *file;
};

/* ������ ������ �������� ��������������� �����������. */
#define LINEWIDTH 75

static struct ascii85 *new_ascii85(FILE *outfile);
static void write_tilemap_header(FILE *stream, int mask);

/**
 * �������������� ��������� ASCII-85 ��� ������ ����� ������
 * � ���� #outfile. ��������� ������� #mask �����������
 * ����������� (�����). ���������� ��������� �� �ţ.
 */
struct ascii85 *
_ascii85_open_tilemap( FILE *outfile, int mask )
{
	struct ascii85 * ctx = new_ascii85( outfile );
	if ( ctx )
		write_tilemap_header( outfile, mask );
	
	return ctx;
}

/**
 * �������������� ��������� ASCII-85 ��� ������ ����� ������
 * � ���� #outfile. ��������� ������� #mask �����������
 * ����������� (�����). �������� �£����� ���
 * _ascii85_open_tilemap(). ���������� ��������� �� ��������,
 * ������� ����� ������������ � ������ ��������.
 */
void *
ascii85_open_tilemap( FILE *outfile, int mask )
{
	return (void *) _ascii85_open_tilemap( outfile, mask );
}


static void write_tonemap_header( FILE *stream );

/**
 * �������������� ��������� ASCII-85 ��� ������ ��������
 * ����������� � ���� #outfile. ���������� ��������� �� �ţ.
 */
struct ascii85 *
_ascii85_open_tonemap( FILE *outfile )
{
	struct ascii85 * ctx = new_ascii85( outfile );
	if ( ctx )
		write_tonemap_header( outfile );
	
	return ctx;
}

/**
 * �������������� ��������� ASCII-85 ��� ������ ��������
 * ����������� � ���� #outfile. �������� �£����� ���
 * _ascii85_open_tonemap(). ���������� ��������� �� ��������,
 * ������� ����� ������������ � ������ ��������.
 */
void *
ascii85_open_tonemap( FILE *outfile )
{
	return (void *) _ascii85_open_tonemap( outfile );
}


static void ascii85_encode(struct ascii85 *a, const unsigned char code);

/**
 * ���������� #zl ������ ����� � ��������� ����� ASCII-85.
 */
static void
_ascii85_write_empty_lines( struct ascii85 *ascii85_p, unsigned int zl )
{
	if (zl) {
		ascii85_encode(ascii85_p, 0xFF);
		ascii85_encode(ascii85_p, 0xFF);
		ascii85_encode(ascii85_p, (unsigned char)((zl >> 8) & 0xFF));
		ascii85_encode(ascii85_p, (unsigned char)(zl & 0xFF));
	}
}

/**
 * ���������� #zl ������ ����� � ����� ASCII-85 (#ctx).
 * �������� �£����� ������ _ascii85_write_empty_lines().
 */
void
ascii85_write_empty_lines( void *ctx, unsigned int zl )
{
	_ascii85_write_empty_lines( (struct ascii85 *) ctx, zl );
}

/**
 * ���������� #z �������������� �������� � ��������� ����� ASCII-85.
 */
static void
_ascii85_write_spaces( struct ascii85 *ascii85_p, unsigned int z )
{
	if (z) {
		/* ���� ����� ������������ ����� 3 ��������, ��
		 * ��� �������� ����� ������������ ����������
		 * ������������ �������. */
		if (z > 3) {
			ascii85_encode(ascii85_p, 0xFF);
			ascii85_encode(ascii85_p, (unsigned char)((z >> 8) & 0xFF));
			ascii85_encode(ascii85_p, (unsigned char)(z & 0xFF));
		} else {
			/* �����, ������������ ������ ���
			 * �������. */
			while (z--) {
				ascii85_encode(ascii85_p, (unsigned char) 0);
			}
		}
	}
}

/**
 * ���������� #z �������������� �������� � ����� ASCII-85 (#ctx).
 * �������� �£����� ������ _ascii85_write_spaces().
 */
void
ascii85_write_spaces( void *ctx, unsigned int z )
{
	_ascii85_write_spaces( (struct ascii85 *) ctx, z );
}

/**
 * ���������� ���������� ����� ����� #tile_index � �������������
 * �������� #tile_area � ��������� ����� ASCII-85.
 */
static void
_ascii85_write_tile( struct ascii85 *ascii85_p,
					 unsigned char tile_index,
					 unsigned char tile_area )
{
	/* ������������ ����� � ������� �����. */
	ascii85_encode(ascii85_p, (unsigned char)tile_index);
	ascii85_encode(ascii85_p, tile_area);
}

/**
 * ���������� ���������� ����� ����� #tile_index � �������������
 * �������� #tile_area � ����� ASCII-85 (#ctx). �������� �£�����
 * ��� _ascii85_write_tile().
 */
void
ascii85_write_tile( void *ctx, unsigned char tile_index,
					unsigned char tile_area )
{
	_ascii85_write_tile( (struct ascii85 *) ctx, tile_index,
						 tile_area );
}

/**
 * ������� ������ �������� ����������� � ���� ������ �������� ASCII base 85.
 * ��������� #a ���������� ��������� ����������� � �������� �����. ������ ��
 * #count ���ޣ��� �� #ss ���� ���������� � ������ #buf. ���� ������ �������
 * ������ #flush, �� ���������� ������ ���������� ������������ � ��������
 * �����.
 */
static void
_ascii85_write_toneline( struct ascii85 *a, char *buf, size_t ss,
						size_t count, int flush )
{

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

/**
 * ������� ������ �������� ����������� � ���� ������ ��������
 * ASCII base 85. �������� �£����� ��� _ascii85_write_toneline().
 */
void
ascii85_write_toneline( void *ctx, char *buf, size_t ss,
						size_t count, int flush )
{
	_ascii85_write_toneline( (struct ascii85 *) ctx,
							 buf, ss, count, flush );
}

static void destroy_ascii85( struct ascii85 *ascii85_p );
static void ascii85_flush( struct ascii85 *a );
static void write_footer( FILE *stream );

/**
 * ��������� ��������� ����� ASCII-85.
 */
static void
_ascii85_close( struct ascii85 *ascii85_p )
{
	/* ����� ������� ������������. */
	ascii85_encode( ascii85_p[c], 0xFF );
	ascii85_encode( ascii85_p[c], 0xFF );
	ascii85_encode( ascii85_p[c], 0xFF );
	ascii85_flush( ascii85_p[c] );

	/* ����� ����������� �����. */
	write_footer( ascii85_p->file );

	/* ������������ ������ */
	destroy_ascii85( ascii85_p );
}

/**
 * ��������� ����� ASCII-85 (#ctx).
 * �������� �£����� ��� _ascii85_close().
 */
void
ascii85_close( void *ctx )
{
	_ascii85_close( (struct ascii85 *) ctx );
}


/* ������� �������. */

/* ������� ��� ������������� ���������, ������������ ��� �������� ����������
 * � ���������� ����������� ��������� ����������� � ���������� �������������.
 * ��������� �������� ������� ����������� � ��������� ������.
 */
static struct ascii85 *
new_ascii85(FILE *outfile) {

	struct ascii85 *a;

	a = (struct ascii85 *) malloc(sizeof(struct ascii85));
	if (a != NULL) {
		a->line_break = LINEWIDTH;
		a->offset = 0;
		a->file = outfile;
		a->buffer[a->offset] = '\0';
	}
	return a;
}

/* ������� ��� ������������ ��������, ������� ���������� ��� �����������
 * �����������.
 */
static void
destroy_ascii85(struct ascii85 *ascii85_p) {
	if (ascii85_p != NULL) {
		free(ascii85_p);
	}
}

/* ������� ��� ����������� ���������� ����� � ������ ��������. ������ ���
 * �������� ������� ���������� ����� ������� � ��������� ��������� �� ������
 * �����. ��������� � ����������� ACSII base 85 ��. PLRM v.3.
 */
static char *
ascii85_tuple(char *tuple, unsigned char *data) {
	
	register long i, x;
	unsigned long code, quantum;

	code = ((((unsigned long) data[0] << 8) | (unsigned long) data[1]) << 16) |
		 ((unsigned long) data[2] << 8) | (unsigned long) data[3];

	if (code == 0L) {
		tuple[0] = 'z';
		tuple[1]='\0';
		return(tuple);
	}
	
	quantum = 85UL*85UL*85UL*85UL;
	for (i = 0; i < 4; i++) {
		x = (long) (code/quantum);
		code -= quantum*x;
		tuple[i] = (char) (x+(int) '!');
		quantum /= 85L;
	}
	tuple[4] = (char) ((code % 85L)+(int) '!');
	tuple[5] = '\0';
	
	return tuple;
}

/* ����������� ���������� ����� � ���������� �������������. ��� �����������
 * ����� ������������ � ������� ��������. ����� ������������ ���������� �������
 * �������� �� ����������� � �����, � ������� ����������� ������ ��������.
 * ������ ������������ � �������� �����, ��������� � ���������.
 */
static void
ascii85_encode(struct ascii85 *a, const unsigned char code) {
	
	long  n;
	register char *q;
	
	if (a == NULL)
		return;

	a->buffer[a->offset] = code;
	a->offset++;
	if (a->offset < 4)
		return;
	
	for (q = ascii85_tuple(a->tuple, a->buffer); *q != '\0'; q++) {
		a->line_break--;
		if ((a->line_break < 0) && (*q != '%')) {
		  if (a->file != NULL) {
		    fputc('\n', a->file);
		  }
			a->line_break = LINEWIDTH;
		}
		if (a->file != NULL) {
		  fputc((unsigned char) *q, a->file);
		}
	}
	
	a->offset = 0;

}

/* ����� ������ �������� � �������� �����. ������������ ��� ���������������
 * ������ �������� ������ �������� ����� ��������� �������� �����������.
 */
static void
ascii85_flush(struct ascii85 *a) {
	
	register char *tuple;
	
	if (a->offset > 0) {
		while (a->offset < 4) {
			a->buffer[a->offset] = '\0';
			a->offset++;
		}
		tuple = ascii85_tuple(a->tuple, a->buffer);
		if (a->file != NULL) {
		  fprintf(a->file, (unsigned char *) (*tuple == 'z' ? "!!!!" : tuple));
		}
	}
	
	if (a->file != NULL) {
	  fprintf(a->file, "~>\n");
	}
}

/**
 * ����� ��������� PostScript-��������� ��� ���������� ������
 * � ����� #stream. ��������� ������� #mask ����������� �����������
 * (�����).
 */
static void
write_tilemap_header(FILE *stream, int mask) {
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
				mask ? "0.0" : "1.0",
				mask ? "negative" : "positive",
				(float) height/hres * 72,
				(float) 72/hres,
				(float) 72/vres);
	}
}

/**
 * ������� � ����� #outfile ��������� ��������� PostScript-���������
 * ������ �������� �����������.
 */
static void
write_tonemap_header( FILE *stream )
{
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
			width, height,
			miniswhite ? "[0 1]" : "[1 0]",
			width, height, height);

}

/**
 * ����� ����������� ����� PostScript-��������� � ��������� �����.
 */
static void
write_footer( FILE *stream ) {
  if (stream != NULL) {
	fprintf(stream, "%%%%EndData\n"
			"grestore\t%% Restore previous graphic state\n");
  }
}
