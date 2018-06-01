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

/* Библиотека для записи многослойных иллюстраций в формате PDF. */

#include "pdfwriter.h"

#include <iostream>
#include <string>
using namespace std;

#include <PDFWriter/PDFWriter.h>
#include <PDFWriter/PDFPage.h>
#include <PDFWriter/PageContentContext.h>
#include <PDFWriter/PDFFormXObject.h>
#include <PDFWriter/TiffUsageParameters.h>
using namespace PDFHummus;

/**
 * Связный список слоёв изображения.
 */
class ObjList {
public:
	string objId;
	ObjList* next;
};

/**
 * Контекст для работы с PDF.
 */
class PDFCtx {
public:
	PDFPage   *pdfPage;
	PageContentContext *pageContentContext;
	double width;
	double height;
	ObjList *objlist;
};

PDFWriter pdfWriter;

/**
 * Открывает PDF файл #filename для записи. Размеры изображения
 * передаются в #width и #height.
 * Возвращает указатель на контекст или #NULL в случае ошибки.
 */
void *
pdf_open_file( const char *filename, double width, double height )
{
	EStatusCode status;
	PDFCtx *ctx;

	ctx = new PDFCtx();
	if ( !ctx ) return NULL;

	ctx->pdfPage = NULL;
	ctx->pageContentContext = NULL;
	ctx->width = width;
	ctx->height = height;
	ctx->objlist = NULL;

	cerr << "Open PDF file: " << filename << "\n";
	status = pdfWriter.StartPDF( filename, ePDFVersion14 );
	if ( status != eSuccess ) {
		cerr << "Error: Unable to open PDF file\n";
		delete ctx;
		return NULL;
	}

	return ctx;
}

static CMYKRGBColor getColor( pdfcolor_t color );
static int addImage( PDFCtx *ctx, PDFFormXObject* image );

/**
 * Добавляет в PDF #ctx микроштрихофой слой из файла #tifffile.
 * Параметр #color определяет цвет штрихов.
 * Возвращает 0 в случае успеха, и не-0 в случае ошибки.
 */
int
pdf_add_bitmap( void *_ctx, const char *tifffile, pdfcolor_t color )
{
	PDFCtx *ctx = (PDFCtx *) _ctx;

	TIFFUsageParameters params;
	params.BWTreatment.AsImageMask = 1;
	params.BWTreatment.OneColor = getColor( color );

	cerr << "Add bitmap file: " << tifffile << " (" << color << ")\n";
	
	PDFFormXObject* image =
		pdfWriter.CreateFormXObjectFromTIFFFile( tifffile, params );
	if ( !image ) {
		cerr << "Error reading the image file!\n";
		return 1;
	}

	int rv = addImage( ctx, image );
	delete image;

	return rv;
}

/**
 * Добавляет в PDF #ctx тоновый слой из файла #tifffile.
 * Параметр #color определяет цвет градаций.
 * Возвращает 0 в случае успеха, и не-0 в случае ошибки.
 */
int
pdf_add_tonemap( void *_ctx, const char *tifffile, pdfcolor_t color )
{
	PDFCtx *ctx = (PDFCtx *) _ctx;

	TIFFUsageParameters params;
	params.GrayscaleTreatment.AsColorMap = true;
	params.GrayscaleTreatment.OneColor = getColor( color );
	params.GrayscaleTreatment.ZeroColor = getColor( PDFCOLOR_WHITE );

	cerr << "Add tonemap file: " << tifffile << " (" << color << ")\n";
	
	PDFFormXObject* image =
		pdfWriter.CreateFormXObjectFromTIFFFile( tifffile, params );
	if ( !image ) {
		cerr << "Error reading the image file!\n";
		return 1;
	}

	int rv = addImage( ctx, image );
	delete image;

	return rv;
}

static int placeImages( PDFCtx *ctx );

/**
 * Закрывает контекст и записывает PDF файл.
 */
int
pdf_close( void *_ctx )
{
	PDFCtx *ctx = (PDFCtx *) _ctx;

	if ( ctx ) {
		if ( placeImages( ctx ) != 0 ) {
			cerr << "Error placing images!\n";
		}
		if ( ctx->pageContentContext ) {
			cerr << "Close PDF page context\n";
			pdfWriter.EndPageContentContext( ctx->pageContentContext );
		}
		if ( ctx->pdfPage ) {
			cerr << "Close PDF page\n";
			pdfWriter.WritePageAndRelease( ctx->pdfPage );
		}

		cerr << "Close PDF\n";
		pdfWriter.EndPDF();
	}

	delete ctx;
}

/**
 * Если страница в контексте отсутствует, добавляет страницу
 * с размерами, указанными при открытии PDF-файла.
 * Возвращает 0 в случае успеха и не-0 иначе.
 */
static int
preparePage( PDFCtx *ctx )
{
	if ( !ctx->pdfPage ) {
		cerr << "Add new page to the PDF\n";
		ctx->pdfPage = new PDFPage();
		if ( !ctx->pdfPage ) {
			cerr << "Error: Unable to add the page\n";
			return 1;
		}
		
		cerr << "Set page media box to " << \
			ctx->width << "x" << ctx->height << "\n";
		ctx->pdfPage->SetMediaBox(
		        PDFRectangle( 0, 0, ctx->width, ctx->height ) );
	}

	return 0;
}

/**
 * Если контекст страницы отсутствует в основном контексте,
 * добавляет его.
 */
static int
preparePageContext( PDFCtx *ctx )
{
	if ( preparePage( ctx ) != 0 ) return 1;
	
	if ( !ctx->pageContentContext ) {
		cerr << "Start new page context\n";
		ctx->pageContentContext = 
			pdfWriter.StartPageContentContext( ctx->pdfPage );
		if ( !ctx->pageContentContext ) {
			cerr << "Error: Unable to start the page context\n";
			return 1;
		}
	}

	return 0;
}

/**
 * Возвращает цвет по его номеру.
 */
static CMYKRGBColor
getColor( pdfcolor_t color )
{
	switch ( color ) {
	case PDFCOLOR_BLACK:
		return CMYKRGBColor( 0, 0, 0, 255 );
	case PDFCOLOR_CYAN:
		return CMYKRGBColor( 255, 0, 0, 0 );
	case PDFCOLOR_MAGENTA:
		return CMYKRGBColor( 0, 255, 0, 0 );
	case PDFCOLOR_YELLOW:
		return CMYKRGBColor( 0, 0, 255, 0 );
	default:
		return CMYKRGBColor( 0, 0, 0, 0 );
	}
}

/**
 * Добавляет изображение к документу.
 */
static int
addImage( PDFCtx *ctx, PDFFormXObject* image )
{
	if ( preparePage( ctx ) != 0 ) return 1;

	string imageId =
		ctx->pdfPage->GetResourcesDictionary().
		      AddFormXObjectMapping( image->GetObjectID() );

	cerr << "Add the image " << imageId << " to the document\n";
	
	ObjList *objlist = ctx->objlist;
	while ( objlist && objlist->next )
		objlist = objlist->next;
	ObjList *newObj = new ObjList();
	newObj->objId = imageId; // TODO: constructor
	newObj->next = NULL;
	if ( objlist )
		objlist->next = newObj;
	else
		ctx->objlist = newObj;
}


/**
 * Помещает изображения на страницу.
 */
static int
placeImages( PDFCtx *ctx )
{
	if ( preparePageContext( ctx ) != 0 ) return 1;

	cerr << "Place the images on the page:";
	
	ctx->pageContentContext->q();
	
	ObjList *objlist = ctx->objlist;
	while ( objlist ) {
		cerr << " " << objlist->objId;
		ctx->pageContentContext->Do( objlist->objId );
		objlist = objlist->next;
	}
	
	ctx->pageContentContext->Q();

	cerr << "\n";

	return 0;
}
