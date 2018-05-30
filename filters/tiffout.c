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


/* ���������� ��� ������ �������� ������ � ������� TIFF. */

#include <stdio.h>
#include <sys/types.h>
#include "system.h"
#include "tiffout.h"
#include <tiff.h>
#include <tiffio.h>
#include "weightfunc.h"

void * tiffout_open_bitmap( const char *outfile, int mask );
void * tiffout_open_tonemap( const char *outfile );
void tiffout_write_tile_lines( void *ctx, unsigned int zl );
void tiffout_write_spaces( void *ctx, unsigned int z );
void tiffout_write_tile( void *ctx, unsigned char tile_index,
						 unsigned char tile_area );
void tiffout_write_toneline( void *ctx, const char *buf,
							 size_t ss, size_t count );
void tiffout_close( void *ctx );

/**
 * ��������� � ��������� ����������� TIFF.
 */
struct filter_writer tiffout_filter_writer = {
	.open_tilemap      = tiffout_open_bitmap,
	.open_tonemap      = tiffout_open_tonemap,
	.write_tile_lines = tiffout_write_tile_lines,
	.write_spaces      = tiffout_write_spaces,
	.write_tile        = tiffout_write_tile,
	.write_toneline    = tiffout_write_toneline,
	.close             = tiffout_close
};

/**
 * ��������� ��� �������� ���������� ��� ������ TIFF-�����.
 */
struct tiffout {
	TIFF *tif;
	unsigned char *buf;
	size_t buflinesize;
	size_t bufsize;
	int y;
	int written;
	int tile_x;
	int is_bitmap;
};


static struct tiffout *new_tiffout(const char *outfile, int bitmap);
static void write_bitmap_header(struct tiffout *tiffout_p, int mask);

/**
 * �������������� ��������� #tiffout ��� ������ ��������� �����������
 * ������ � ���� #outfile. ��������� ������� #mask �����������
 * ����������� (�����). ���������� ��������� �� ���������.
 */
struct tiffout *
_tiffout_open_bitmap( const char *outfile, int mask )
{
	struct tiffout * ctx = new_tiffout( outfile, 1 );
	if ( ctx ) {		
		write_bitmap_header( ctx, mask );
	}
	
	return ctx;
}

/**
 * �������������� ��������� #tiffout ��� ������ ��������� �����������
 * ������ � ���� #outfile. ��������� ������� #mask �����������
 * ����������� (�����). �������� �£����� ���
 * _tiffout_open_bitmap(). ���������� ��������� �� ��������,
 * ������� ����� ������������ � ������ ��������.
 */
void *
tiffout_open_bitmap( const char *outfile, int mask )
{
	weightfuncs_init();
	return (void *) _tiffout_open_bitmap( outfile, mask );
}


static void write_tonemap_header( struct tiffout *tiffout_p );

/**
 * �������������� ��������� #tiffout ��� ������ ��������
 * ����������� � ���� #outfile. ���������� ��������� �� �ţ.
 */
struct tiffout *
_tiffout_open_tonemap( const char *outfile )
{
	struct tiffout * ctx = new_tiffout( outfile, 0 );
	if ( ctx )
		write_tonemap_header( ctx );
	
	return ctx;
}

/**
 * �������������� ��������� #tiffout ��� ������ ��������
 * ����������� � ���� #outfile. �������� �£����� ���
 * _tiffout_open_tonemap(). ���������� ��������� �� ��������,
 * ������� ����� ������������ � ������ ��������.
 */
void *
tiffout_open_tonemap( const char *outfile )
{
	return (void *) _tiffout_open_tonemap( outfile );
}

static void tiffout_flush( struct tiffout *a );

/**
 * ���������� #zl ����� ������ � ��������� �����������
 * #tiffout. ���� #zl > 1, �� ������������� ������������ ������
 * ������.
 */
static void
_tiffout_write_tile_lines( struct tiffout *a, unsigned int zl )
{
	if ( zl ) {
		tiffout_flush( a );
		memset( a->buf, 0, a->bufsize );
		zl--;
	
		while ( zl ) {
			a->written = 0;
			tiffout_flush( a );
			zl--;
		}
	}
}

/**
 * ���������� #zl ����� ������ � ����������� #ctx.
 * �������� �£����� ������ _tiffout_write_tile_lines().
 */
void
tiffout_write_tile_lines( void *ctx, unsigned int zl )
{
	_tiffout_write_tile_lines( (struct tiffout *) ctx, zl );
}

static void _tiffout_write_tile( struct tiffout *a,
								 unsigned char tile_index,
								 unsigned char tile_area );

/**
 * ���������� #z ������ ������ � ��������� �����������
 * #tiffout.
 */
static void
_tiffout_write_spaces( struct tiffout *a, unsigned int z )
{
	while ( z ) {
		_tiffout_write_tile( a, 0, 0 );
		z--;
	}
}

/**
 * ���������� #z ������ ������ � ���� #ctx.
 * �������� �£����� ������ _tiffout_write_spaces().
 */
void
tiffout_write_spaces( void *ctx, unsigned int z )
{
	_tiffout_write_spaces( (struct tiffout *) ctx, z );
}

static void get_tile_bytemap( unsigned char *tilebuf,
							  unsigned char tile_index,
							  unsigned char tile_area );
static void tiffout_encode_tile( struct tiffout *a,
								 unsigned char *tilebuf );

/**
 * ��������� ���� � ������� #tile_index � �������������
 * �������� #tile_area � ��������� ����������� #tiffout.
 */
static void
_tiffout_write_tile( struct tiffout *a,
					 unsigned char tile_index,
					 unsigned char tile_area )
{
	static unsigned char tilebuf[WEIGHTFUNC_LEN];
	get_tile_bytemap( tilebuf, tile_index, tile_area );
	tiffout_encode_tile( a, tilebuf );
}

/**
 * ��������� ���� � ������� #tile_index � �������������
 * �������� #tile_area � ��������� ����������� #ctx.
 * �������� �£����� ��� _tiffout_write_tile().
 */
void
tiffout_write_tile( void *ctx, unsigned char tile_index,
					unsigned char tile_area )
{
	_tiffout_write_tile( (struct tiffout *) ctx, tile_index,
						 tile_area );
}

/**
 * ������� ������ �������� ����������� � ��������� ���� #tiffout.
 *������ �� #count ���ޣ��� �� #ss ���� ���������� � ������ #buf.
 */
static void
_tiffout_write_toneline( struct tiffout *a, const char *buf,
						 size_t ss, size_t count )
{
	if ( ss != 1 && count != width ) {
		fprintf( stderr, "Error: Wrong tone line: %d x %d\n",
				 ss, count );
		return;
	}
	TIFFWriteScanline( a->tif, buf, a->y, 0);
	a->y = a->y + 1;
}

/**
 * ������� ������ �������� ����������� � ��������� ���� #ctx.
 * �������� �£����� ��� _tiffout_write_toneline().
 */
void
tiffout_write_toneline( void *ctx, const char *buf, size_t ss,
						size_t count )
{
	_tiffout_write_toneline( (struct tiffout *) ctx,
							 buf, ss, count );
}

static void destroy_tiffout( struct tiffout *tiffout_p );

/**
 * ��������� ��������� ����� #tiffout.
 */
static void
_tiffout_close( struct tiffout *tiffout_p )
{
	/* ����� ������� ������������. */
	tiffout_flush( tiffout_p );
	
	/* �������� �����. */
	TIFFClose( tiffout_p->tif );

	/* ������������ ������ */
	destroy_tiffout( tiffout_p );
}

/**
 * ��������� ����� #ctx.
 * �������� �£����� ��� _tiffout_close().
 */
void
tiffout_close( void *ctx )
{
	_tiffout_close( (struct tiffout *) ctx );
}


/* ������� �������. */

/**
 * �������������� ��������� #tiffout, ������� ������������ �� �����
 * ����������� ������ � �������� ������������� � �� ����� ������
 * �������� ����������� � ���� ������� TIFF.
 * ��������� �������� TIFF c ��������� #outfile.
 */
static struct tiffout *
new_tiffout( const char *outfile, int bitmap ) {

	struct tiffout *a;

	a = (struct tiffout *) malloc( sizeof(struct tiffout) );
	
	if (a != NULL) {
		a->y = 0;
		a->written = 1;
		a->tile_x = 0;
		a->is_bitmap = bitmap;
		if ( bitmap ) {
			a->buflinesize = (width * TILEWIDTH) / 8;
			a->buflinesize += ((width * TILEWIDTH) % 8) ? 1 : 0;
			a->bufsize = a->buflinesize * TILEHEIGHT;
		} else {
			a->buflinesize = width;
			a->bufsize = a->buflinesize;
		}
		a->buf = _TIFFmalloc( a->bufsize );
		if ( !a->buf ) {
			fprintf( stderr, "Unable to create allocate memory\n" );
			free( a );
			a = NULL;
		} else {
			memset( a->buf, 0, a->bufsize );
			a->tif = TIFFOpen( outfile, "w" );
			if ( a->tif == NULL ) {
				fprintf( stderr, "Unable to create TIFF file %s\n",
						 outfile );
				_TIFFfree( a->buf );
				free( a );
				a = NULL;
			}
		}
	}
	
	return a;
}

/**
 * ������� ��� ������������ ��������, ������� ���������� ���
 * ����������� �����������.
 */
static void
destroy_tiffout( struct tiffout *tiffout_p ) {
	if ( tiffout_p ) {
		if ( tiffout_p->buf ) {
			_TIFFfree( tiffout_p->buf );
		}
		free( tiffout_p );
	}
}

/* ������ � TIFF �ݣ �� ���������� �����.
 */
static void
tiffout_flush( struct tiffout *a )
{
	int y;

	if ( !a->written ) {
		for ( y = 0; y < (a->is_bitmap ? TILEHEIGHT : 1); y++ ) {
			TIFFWriteScanline( a->tif, a->buf + y * a->buflinesize,
							   a->y, 0);
			a->y = a->y + 1;
		}
		a->written = 1;
		a->tile_x = 0;
	}
}

/**
 * ����� ��������� ��� ������ ��������� ����������� � TIFF-����,
 * ��������� � #tiffout_p. ��������� ������� #mask �����������
 * ����������� (�����).
 */
static void
write_bitmap_header( struct tiffout *tiffout_p, int mask ) {
	TIFFSetField( tiffout_p->tif, TIFFTAG_SOFTWARE,
				  PACKAGE " v" VERSION);
	TIFFSetField( tiffout_p->tif, TIFFTAG_PLANARCONFIG,
				  PLANARCONFIG_CONTIG );
	TIFFSetField( tiffout_p->tif, TIFFTAG_SAMPLESPERPIXEL, 1 );
	TIFFSetField( tiffout_p->tif, TIFFTAG_BITSPERSAMPLE, 1 );
	TIFFSetField( tiffout_p->tif, TIFFTAG_IMAGEWIDTH,
				  width * TILEWIDTH);
	TIFFSetField( tiffout_p->tif, TIFFTAG_IMAGELENGTH,
				  height * TILEHEIGHT );
	TIFFSetField( tiffout_p->tif, TIFFTAG_XRESOLUTION,
				  hres * TILEWIDTH );
	TIFFSetField( tiffout_p->tif, TIFFTAG_YRESOLUTION,
				  vres * TILEHEIGHT );
	TIFFSetField( tiffout_p->tif, TIFFTAG_PHOTOMETRIC,
				  PHOTOMETRIC_MINISWHITE );
	TIFFSetField( tiffout_p->tif, TIFFTAG_RESOLUTIONUNIT,
				  RESUNIT_INCH );
}

/**
 * ����� ��������� ��� ������ �������� ����������� � TIFF-����,
 * ��������� � #tiffout_p.
 */
static void
write_tonemap_header( struct tiffout *tiffout_p )
{
	TIFFSetField( tiffout_p->tif, TIFFTAG_SOFTWARE,
				  PACKAGE " v" VERSION);
	TIFFSetField( tiffout_p->tif, TIFFTAG_PLANARCONFIG,
				  PLANARCONFIG_CONTIG );
	TIFFSetField( tiffout_p->tif, TIFFTAG_SAMPLESPERPIXEL, 1 );
	TIFFSetField( tiffout_p->tif, TIFFTAG_BITSPERSAMPLE, 8 );
	TIFFSetField( tiffout_p->tif, TIFFTAG_IMAGEWIDTH, width );
	TIFFSetField( tiffout_p->tif, TIFFTAG_IMAGELENGTH, height );
	TIFFSetField( tiffout_p->tif, TIFFTAG_XRESOLUTION, hres );
	TIFFSetField( tiffout_p->tif, TIFFTAG_YRESOLUTION, vres );
	TIFFSetField( tiffout_p->tif, TIFFTAG_PHOTOMETRIC, 
				  miniswhite ? PHOTOMETRIC_MINISWHITE :
				               PHOTOMETRIC_MINISBLACK );
	TIFFSetField( tiffout_p->tif, TIFFTAG_RESOLUTIONUNIT,
				  RESUNIT_INCH );
}


static void get_tile_bytemap( unsigned char *tilebuf,
							  unsigned char tile_index,
							  unsigned char tile_area )
{
	weight_func_apply( tilebuf, tile_index, tile_area );
}

static void tiffout_encode_tile( struct tiffout *a,
								 unsigned char *tilebuf )
{
	int i, j;
	unsigned char *p;

	if ( a->tile_x >= width ) {
		fprintf( stderr, "Error: tile X too big: %d\n", a->tile_x );
		return;
	}
	
	int byte_offs = (a->tile_x * TILEWIDTH) / 8;
	int bit_offs = (a->tile_x * TILEWIDTH) % 8;

	for ( j = 0; j < TILEHEIGHT; j++ ) {
		p = a->buf + (a->buflinesize * j) + byte_offs;
		int _bit_offs = bit_offs;
		for ( i = 0; i < TILEWIDTH; i++ ) {
			int shift = 7 - i - _bit_offs;
			if ( shift < 0 ) {
				p++;
				_bit_offs -= 8;
				shift = 8 + shift;
			}
			if ( tilebuf[ j * TILEWIDTH + i ] ) {
				*p |= 1 << shift;
			} else {
				*p &= ~(1 << shift);
			}
		}
	}

	a->tile_x = a->tile_x + 1;
	a->written = 0;
}
