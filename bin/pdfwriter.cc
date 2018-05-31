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

// standard library includes
#include <iostream>
#include <string>
using namespace std;
// end standard library includes

// pdfwriter library includes
#include <PDFWriter/PDFWriter.h>
#include <PDFWriter/PDFPage.h>
#include <PDFWriter/PageContentContext.h>
#include <PDFWriter/PDFFormXObject.h>
#include <PDFWriter/TiffUsageParameters.h>
// end pdfwriter library includes

using namespace PDFHummus;

/**
 * �������� ��� ������ � PDF.
 */
class PDFCtx {
public:
	PDFPage   *pdfPage;
	PageContentContext *pageContentContext;
};

PDFWriter pdfWriter;

/**
 * ��������� PDF ���� #filename ��� ������.
 * ���������� ��������� �� �������� ��� #NULL � ������ ������.
 */
void *
pdf_open_file( const char *filename )
{
	EStatusCode status;
	PDFCtx *ctx;

	ctx = new PDFCtx();
	if ( !ctx ) return NULL;

	ctx->pdfPage = NULL;
	ctx->pageContentContext = NULL;

	status = pdfWriter.StartPDF( filename, ePDFVersion14 );
	if ( status != eSuccess ) {
		delete ctx;
		return NULL;
	}

	return ctx;
}

/**
 * ��������� � PDF #ctx �������������� ���� �� ����� #tifffile.
 * �������� #color ���������� ���� �������.
 * ���������� 0 � ������ ������, � ��-0 � ������ ������.
 */
int
pdf_add_bitmap( void *_ctx, const char *tifffile, pdfcolor_t color )
{
	PDFCtx *ctx = (PDFCtx *) _ctx;
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
