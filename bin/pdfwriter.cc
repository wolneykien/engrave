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

/* ���������� ��� ������ ������������ ����������� � ������� PDF. */

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
 * �������� ��� ������ � PDF.
 */
class PDFCtx {
public:
	PDFPage   *pdfPage;
	PageContentContext *pageContentContext;
	double width;
	double height;
};

PDFWriter pdfWriter;

/**
 * ��������� PDF ���� #filename ��� ������. ������� �����������
 * ���������� � #width � #height.
 * ���������� ��������� �� �������� ��� #NULL � ������ ������.
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

	status = pdfWriter.StartPDF( filename, ePDFVersion14 );
	if ( status != eSuccess ) {
		delete ctx;
		return NULL;
	}

	return ctx;
}

static int preparePage( PDFCtx *ctx );
static int preparePageContext( PDFCtx *ctx );
static CMYKRGBColor getColor( pdfcolor_t color );
static int placeImage( PDFCtx *ctx, PDFFormXObject* image );

/**
 * ��������� � PDF #ctx �������������� ���� �� ����� #tifffile.
 * �������� #color ���������� ���� �������.
 * ���������� 0 � ������ ������, � ��-0 � ������ ������.
 */
int
pdf_add_bitmap( void *_ctx, const char *tifffile, pdfcolor_t color )
{
	PDFCtx *ctx = (PDFCtx *) _ctx;

	TIFFUsageParameters params;
	params.BWTreatment.AsImageMask = 1;
	params.BWTreatment.OneColor = getColor( color );
	
	PDFFormXObject* image =
		pdfWriter.CreateFormXObjectFromTIFFFile( tifffile, params );
	if ( !image ) return 1;
	
	int rv = placeImage( ctx, image );
	delete image;

	return rv;
}

/**
 * ��������� � PDF #ctx ������� ���� �� ����� #tifffile.
 * �������� #color ���������� ���� ��������.
 * ���������� 0 � ������ ������, � ��-0 � ������ ������.
 */
int
pdf_add_tonemap( void *_ctx, const char *tifffile, pdfcolor_t color )
{
	PDFCtx *ctx = (PDFCtx *) _ctx;

	TIFFUsageParameters params;
	params.GrayscaleTreatment.AsColorMap = true;
	params.GrayscaleTreatment.OneColor = getColor( color );
	params.GrayscaleTreatment.ZeroColor = getColor( PDFCOLOR_WHITE );
	
	PDFFormXObject* image =
		pdfWriter.CreateFormXObjectFromTIFFFile( tifffile, params );
	if ( !image ) return 1;
	
	int rv = placeImage( ctx, image );
	delete image;

	return rv;
}

/**
 * ��������� �������� � ���������� PDF ����.
 */
int
pdf_close( void *_ctx )
{
	PDFCtx *ctx = (PDFCtx *) _ctx;

	if ( ctx ) {
		if ( ctx->pageContentContext )
			pdfWriter.EndPageContentContext( ctx->pageContentContext );
		
		if ( ctx->pdfPage )
			pdfWriter.WritePageAndRelease( ctx->pdfPage );
		
		pdfWriter.EndPDF();
	}

	delete ctx;
}

/**
 * ���� �������� � ��������� �����������, ��������� ��������
 * � ���������, ���������� ��� �������� PDF-�����.
 * ���������� 0 � ������ ������ � ��-0 �����.
 */
static int
preparePage( PDFCtx *ctx )
{
	if ( !ctx->pdfPage ) {
		ctx->pdfPage = new PDFPage();
		if ( !ctx->pdfPage ) return 1;
		ctx->pdfPage->SetMediaBox(
		        PDFRectangle( 0, 0, ctx->width, ctx->height ) );
	}

	return 0;
}

/**
 * ���� �������� �������� ����������� � �������� ���������,
 * ��������� ���.
 */
static int
preparePageContext( PDFCtx *ctx )
{
	if ( preparePage( ctx ) != 0 ) return 1;
	
	if ( !ctx->pageContentContext ) {
		ctx->pageContentContext = 
			pdfWriter.StartPageContentContext( ctx->pdfPage );
		if ( !ctx->pageContentContext ) return 1;
	}

	return 0;
}

/**
 * ���������� ���� �� ��� ������.
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
 * �������� ����������� �� ��������.
 */
static int
placeImage( PDFCtx *ctx, PDFFormXObject* image )
{
	if ( preparePageContext( ctx ) != 0 ) return 1;
	
	ctx->pageContentContext->q();
	ctx->pageContentContext->Do(
          ctx->pdfPage->GetResourcesDictionary().
		      AddFormXObjectMapping(
			      image->GetObjectID() ) );
	ctx->pageContentContext->Q();

	return 0;
}
