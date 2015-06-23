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


/* Константы и интерфейс функций библиотеки, реализующих анализ участка
 * изображения. */

#include "system.h"

/* Коды -- номера базовых тайлов. */
#define TILE_NL 1
#define TILE_WL 2
#define TILE_NWL 3
#define TILE_NEL 4
#define TILE_WS 5
#define TILE_NS 6
#define TILE_ES 7
#define TILE_SS 8
#define TILE_NES 9
#define TILE_SES 10
#define TILE_SWS 11
#define TILE_NWS 12
#define TILE_WC 13
#define TILE_NC 14
#define TILE_EC 15
#define TILE_SC 16
#define TILE_NWC 17
#define TILE_NEC 18
#define TILE_SEC 19
#define TILE_SWC 20

/* Общее количество базовых тайлов. */
#define TILE_COUNT 20

/* Коды направлений. */

#define AE 7
#define BE 0
#define CE 1
#define DE 6
#define FE 2
#define GE 5
#define HE 4
#define IE 3

/* Макроопределения для облегчения доступа к компонентом окна отсётов. */
#define pA window[1][1]
#define pB window[1][2]
#define pC window[1][3]
#define pD window[2][1]
#define pE window[2][2]
#define pF window[2][3]
#define pG window[3][1]
#define pH window[3][2]
#define pI window[3][3]
#define pB1 window[0][2]
#define pD1 window[2][0]
#define pH1 window[4][2]
#define pF1 window[2][4]

#define A *pA
#define B *pB
#define C *pC
#define D *pD
#define E *pE
#define F *pF
#define G *pG
#define H *pH
#define I *pI
#define B1 *pB1
#define D1 *pD1
#define H1 *pH1
#define F1 *pF1

/* Определение типа данных для окна отсчётов как двумерного массива указателей
 * на отсчёты изображения. */
typedef unsigned char *t_window[5][5];

/* Набор счётчиков пустых участков и строк, используемый при построении
 * штрихового изображения из тайлов. */
struct maketiles_info {
	unsigned int pz;
	unsigned int nz;
	unsigned int pzl;
	unsigned int nzl;
};


/* Параметры алгоритма анализа. */

/* Признак проверки пересечения штрихом центральной области окрестности. */
extern int outtest;

/* Порог сравнения отдельных отсчётов (0 - 255). */
extern unsigned char FThr;

/* Порог сравнения суммарных значений отсчётов (0 - 3*255). */
extern double FThr2;

/* Коэффициент корреляции для диагонально расположенных отсчётов. */
extern double FDcor;

/* Минимальная площадь штриха. */
extern unsigned char minarea;

/* Интерфейс библиотеки. */

void setOutSens(int value);
int getOutSens();
void setFThr(unsigned char value);
unsigned char getFThr();
void setFThr2(double value);
double getFThr2();
void setFDCor(double value);
double getFDCor();
void set_qMethod(int value);
int get_qMethod();
void set_too_thin_line(unsigned char value);
unsigned char get_too_thin_line();

void edgecpy(char *buf, size_t width, size_t ss);
void init_maketiles_info(struct maketiles_info *mi);

void get_tile(t_window window, int *neg, int *tile_index, unsigned char *tile_area, unsigned char *bg_value);

