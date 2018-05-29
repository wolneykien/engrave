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


/* Библиотека для записи бинарных данных в формате TIFF. */

#include <stdio.h>
#include <sys/types.h>
#include "system.h"
#include "tiffout.h"
#include <tiff.h>
#include <tiffio.h>

void * tiffout_open_bitmap( const char *outfile, int mask );
void * tiffout_open_tonemap( const char *outfile );
void tiffout_write_empty_lines( void *ctx, unsigned int zl );
void tiffout_write_spaces( void *ctx, unsigned int z );
void tiffout_write_tile( void *ctx, unsigned char tile_index,
						 unsigned char tile_area );
void tiffout_write_toneline( void *ctx, char *buf, size_t ss,
								 size_t count, int flush );
void tiffout_close( void *ctx );

/**
 * Структура с функциями кодировщика TIFF.
 */
struct filter_writer tiffout_filter_writer = {
	.open_tilemap      = tiffout_open_bitmap,
	.open_tonemap      = tiffout_open_tonemap,
	.write_empty_lines = tiffout_write_empty_lines,
	.write_spaces      = tiffout_write_spaces,
	.write_tile        = tiffout_write_tile,
	.write_toneline    = tiffout_write_toneline,
	.close             = tiffout_close
};

/**
 * Структура для хранения информации для записи TIFF-файла.
 */
struct tiffout {
	TIFF *tif;
	int is_bitmap;
};

/* Ширина тайла в пикселях. */
#define TILEWIDTH 6

/* Высота тайла в пикселях. */
#define TILEHEIGHT 6


static struct tiffout *new_tiffout(const char *outfile);
static void write_bitmap_header(struct tiffout *tiffout_p, int mask);

/**
 * Инициализирует структуру #tiffout для записи бинарного изображения
 * тайлов в файл #outfile. Принимает признак #mask негативного
 * изображения (маски). Возвращает указатель на структуру.
 */
struct tiffout *
_tiffout_open_bitmap( const char *outfile, int mask )
{
	struct tiffout * ctx = new_tiffout( outfile );
	if ( ctx ) {
		ctx->is_bitmap = 1;
		write_bitmap_header( ctx, mask );
	}
	
	return ctx;
}

/**
 * Инициализирует структуру #tiffout для записи бинарного изображения
 * тайлов в файл #outfile. Принимает признак #mask негативного
 * изображения (маски). Является обёрткой над
 * _tiffout_open_bitmap(). Возвращает указатель на контекст,
 * который затем используется в других функциях.
 */
void *
tiffout_open_bitmap( const char *outfile, int mask )
{
	return (void *) _tiffout_open_bitmap( outfile, mask );
}


static void write_tonemap_header( struct tiffout *tiffout_p );

/**
 * Инициализирует структуру #tiffout для записи тонового
 * изображения в файл #outfile. Возвращает указатель на неё.
 */
struct tiffout *
_tiffout_open_tonemap( const char *outfile )
{
	struct tiffout * ctx = new_tiffout( outfile );
	if ( ctx )
		write_tonemap_header( ctx );
	
	return ctx;
}

/**
 * Инициализирует структуру #tiffout для записи тонового
 * изображения в файл #outfile. Является обёрткой над
 * _tiffout_open_tonemap(). Возвращает указатель на контекст,
 * который затем используется в других функциях.
 */
void *
tiffout_open_tonemap( const char *outfile )
{
	return (void *) _tiffout_open_tonemap( outfile );
}


static void tiffout_encode(struct tiffout *a, const unsigned char code);

/**
 * Записывает #zl пустых строк тайлов в указанное изображение
 * #tiffout.
 */
static void
_tiffout_write_empty_lines( struct tiffout *tiffout_p, unsigned int zl )
{
	if (zl) {
		// TODO
	}
}

/**
 * Записывает #zl пустых строк тайлов в изображение #ctx.
 * Является обёрткой вокруг _tiffout_write_empty_lines().
 */
void
tiffout_write_empty_lines( void *ctx, unsigned int zl )
{
	_tiffout_write_empty_lines( (struct tiffout *) ctx, zl );
}

/**
 * Записывает #z пустых тайлов в указанное изображение
 * #tiffout.
 */
static void
_tiffout_write_spaces( struct tiffout *tiffout_p, unsigned int z )
{
	if (z) {
		// TODO
	}
}

/**
 * Записывает #z пустых тайлов в файл #ctx.
 * Является обёрткой вокруг _tiffout_write_spaces().
 */
void
tiffout_write_spaces( void *ctx, unsigned int z )
{
	_tiffout_write_spaces( (struct tiffout *) ctx, z );
}

/**
 * Формирует тайл с номером #tile_index и относительной
 * площадью #tile_area в указанном изображении #tiffout.
 */
static void
_tiffout_write_tile( struct tiffout *tiffout_p,
					 unsigned char tile_index,
					 unsigned char tile_area )
{
	//TODO
}

/**
 * Формирует тайл с номером #tile_index и относительной
 * площадью #tile_area в указанном изображении #ctx.
 * Является обёрткой над _tiffout_write_tile().
 */
void
tiffout_write_tile( void *ctx, unsigned char tile_index,
					unsigned char tile_area )
{
	_tiffout_write_tile( (struct tiffout *) ctx, tile_index,
						 tile_area );
}

static void tiffout_flush( struct tiffout *a );

/**
 * Выводит строку тонового изображения в указанный файл #tiffout. Строка из
 * #count отсчётов по #ss байт передаётся в буфере #buf. Если указан признак
 * сброса #flush, то содержимое буфера коировщика сбрасывается в выходной
 * поток.
 */
static void
_tiffout_write_toneline( struct tiffout *a, char *buf, size_t ss,
						size_t count, int flush )
{
	//TODO
	
	/* Если признак сброса установлен, сбросить буфер кодировщика в
	 * выходной поток.
	 */
	if (flush)
		tiffout_flush(a);

}

/**
 * Выводит строку тонового изображения в указанный файл #ctx.
 * Является обёрткой над _tiffout_write_toneline().
 */
void
tiffout_write_toneline( void *ctx, char *buf, size_t ss,
						size_t count, int flush )
{
	_tiffout_write_toneline( (struct tiffout *) ctx,
							 buf, ss, count, flush );
}

static void destroy_tiffout( struct tiffout *tiffout_p );

/**
 * Закрывает указанный буфер #tiffout.
 */
static void
_tiffout_close( struct tiffout *tiffout_p )
{
	if ( tiffout_p->is_bitmap ) {
		/* Сброс буферов кодировщиков. */
		tiffout_flush( tiffout_p );
	}
	
	/* Закрытие файла. */
	TIFFClose( tiffout_p->tif );

	/* Освобождение памяти */
	destroy_tiffout( tiffout_p );
}

/**
 * Закрывает буфер #ctx.
 * Является обёрткой над _tiffout_close().
 */
void
tiffout_close( void *ctx )
{
	_tiffout_close( (struct tiffout *) ctx );
}


/* Базовые функции. */

/**
 * Инициализирует структуру #tiffout, которая используется во время
 * кодирования тайлов в бинарное представление и во время записи
 * тонового изображения в файл формата TIFF.
 * Структура связыает TIFF c указанным #outfile.
 */
static struct tiffout *
new_tiffout( const char *outfile ) {

	struct tiffout *a;

	a = (struct tiffout *) malloc(sizeof(struct tiffout));
	if (a != NULL) {
		a->is_bitmap = 0;
		a->tif = TIFFOpen( outfile, "w" );
		if ( a->tif == NULL ) {
			fprintf( stderr, "Unable to create TIFF file %s\n",
					 outfile );
			free( a );
			a = NULL;
		}
	}
	return a;
}

/**
 * Функция для освобождения ресурсов, занятых структурой для
 * кодирования изображений.
 */
static void
destroy_tiffout( struct tiffout *tiffout_p ) {
	if ( tiffout_p != NULL ) {
		free( tiffout_p );
	}
}

/* Сброс буфера символов в выходной поток. Используется для принудительного
 * вывода неполной строки символов после окончания процесса кодирования.
 */
static void
tiffout_flush( struct tiffout *a ) {
	TIFFFlush( a->tif );
}

/**
 * Вывод заголовка для записи бинарного изображения в TIFF-файл,
 * связанный с #tiffout_p. Принимает признак #mask негативного
 * изображения (маски).
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
				  mask ? PHOTOMETRIC_MINISBLACK :
				         PHOTOMETRIC_MINISWHITE );
	TIFFSetField( tiffout_p->tif, TIFFTAG_RESOLUTIONUNIT,
				  RESUNIT_INCH );
}

/**
 * Вывод заголовка для записи тонового изображения в TIFF-файл,
 * связанный с #tiffout_p.
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
