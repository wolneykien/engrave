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


/* Библиотека функций для анализа окна отсчётов. */

#include "tile32f.h"
#include <stdio.h>
#include <math.h>

/* Параметры алгоритма анализа. */

/* Признак проверки пересечения штрихом центральной области окрестности. */
int outtest = 1;

/* Порог сравнения отдельных отсчётов (0 - 255). */
unsigned char FThr = 13;

/* Порог сравнения суммарных значений отсчётов (0 - 3*255). */
double FThr2 = 38.4;

/* Коэффициент корреляции для диагонально расположенных отсчётов. */
double FDcor = 1.0;

/* Минимальная площадь штриха. */
unsigned char minarea = 1;

/* Описание функций библиотеки. */

/* Дублирование крайних отсчётов строки изображения, расположенной в
 * указанном буфере и состоящей из отсчётов указанного размера и указанного
 * количества таких отсчётов. */
void edgecpy(char *buf, size_t width, size_t ss) {

	memcpy(buf, buf+2*ss, ss);
	memcpy(buf+ss, buf, ss);
	memcpy(buf+2*ss+width*ss, buf+2*ss+width*ss-ss, ss);
	memcpy(buf+2*ss+width*ss+ss, buf+2*ss+width*ss, ss);

}

/* Функция для поиска наибольшей по модулю суммы. */
int
amax(double *V)
{
	int i;
	double m;
	int res;

	/* Пороговое значение для сравнения сумм. */
	m = FThr2;
	res = 0;

	i = 0;
	while (i < 8) {
	  if (abs(V[i]) > abs(m)) {
	    res = i;
	    m = V[i];
	  }
	  i++;
	}

	return res;

}

/* Вспомогательная функция: поиск максимального значения
 * в окрестности. */
unsigned char max(t_window window) {
			
  unsigned char res = 0;
			
  if (A > res)
    res = A;
  if (B > res)
    res = B;
  if (C > res)
    res = C;
  if (D > res)
    res = D;
  if (E > res)
    res = E;
  if (F > res)
    res = F;
  if (G > res)
    res = G;
  if (H > res)
    res = H;
  if (I > res)
    res = I;
  if ((res - E) < FThr)
    res = E;
  if ((255 - res) < FThr)
    res = 255;

  return res;
			
}
		
/* Вспомогательная функция: поиск минимального значения
 * в окрестности. */
unsigned char min(t_window window) {

  unsigned char res;

  res = 255;
			
  if (A < res)
    res = A;
  if (B < res)
    res = B;
  if (C < res)
    res = C;
  if (D < res)
    res = D;
  if (E < res)
    res = E;
  if (F < res)
    res = F;
  if (G < res)
    res = G;
  if (H < res)
    res = H;
  if (I < res)
    res = I;
  if ((E - res)<FThr)
    res = E;
  if (res < FThr)
    res = 0;

  return res;
}

/* Анализ изображения с получением номера и относительной площади
 * тайла. Кроме окна отсчётов, указывается признак инверсии. */
int get_tile_index(t_window window, int *negfig, int *inverse) {

	/* Суммарные значения отсчётов по 8-ми направлениям. */
	double V[8];

	/* Индекс наибольшей по модулю суммы. */
	int m;

	/* Коэффициент корреляции для отсчётов в окне. */
	double ks;

	/* Вспомогательные функции. */

	/* Функция сравнения суммарных значений. Указываются индексы
	 * предварительно вычесленных суммарных значений. */
	int equ(int i1, int i2) {
		if ((V[i1] > 0 && V[i2] < 0) || (V[i1] < 0 && V[i2] > 0))
			return 0;
		else
		  if ((abs(V[i1]) < FThr2 && abs(V[i2]) > FThr2) || (abs(V[i1]) > FThr2 && abs(V[i2]) < FThr2))
				return 0;
			else
				if (abs( abs(V[i1]) - abs(V[i2]) ) < FThr2)
					return 1;
				else
					return 0;
	}

	/* Определить, одинаково ли направление указанных
	 *  градиентов.
	 *//*
	int iscodir(int i1, int i2) {
	  if (V[i1] > 0 && V[i2] < 0) {
	    return 1;
	  }
	  if (V[i1] < 0 && V[i2] > 0) {
	    return 1;
	  }
	  return 0;
	  }*/

	/* Выбрать первый тайл, если направления указанных градиентов
	 *  совпадают, иначе -- второй.
	 *//*
	int codir(int i1, int i2, int t1, int t2) {
	  if (iscodir(i1, i2)) {
	    return t1;
	  } else {
	    return t2;
	  }
	  }*/

	/* Определение того, что обе указанные разности между
	 * указанными значениями и центральным значением превышают
	 * порог. */
	int bGZ(unsigned char s1, unsigned char s2) {
		return ((s1-E) > FThr && (s2-E) > FThr);
	}

	/* Определение того, что обе разности между указанными
	 * значениями и центральным значением меньше
	 * отрицательного значения порога. */
	int bLZ(unsigned char s1, unsigned char s2) {
		return ( (s1-E) < -FThr && (s2-E) < -FThr);
	}

	/* Сравнение значенй отдельных отсчётов с учётом порога. */
	int equByte(unsigned char s1, unsigned char s2) {
		return (abs(s1-s2) < FThr);
	}

	/* Возвращает знак аргумента с учётом порога. */
	int tsign(int dif) {
	  if (dif < -FThr)
	    return -1;
	  else if (dif > FThr)
	    return 1;
	  else
	    return 0;
	}

	/* Проверка пересечения контуром центрального участка части
	 * окрестности. */
	int COut(int m) {
		/* Анализ более обширной окрестности. */
		if (equByte(E,D) && equByte(E,B)) {
			if (!equByte(E,F) && tsign(E-F) > 0 && tsign(D-D1) > 0) {
				*negfig = 1;
				return 0;
			} else if (!equByte(E,F) && tsign(E-F) < 0 && tsign(D-D1) < 0) {
				*negfig = 0;
				return 0;
			} else if (!equByte(E,H) && tsign(E-H) > 0 && tsign(B-B1) > 0) {
				*negfig = 1;
				return 0;
			} else if (!equByte(E,H) && tsign(E-H) < 0 && tsign(B-B1) < 0) {
				*negfig = 0;
				return 0;
			} else {
				return 1;
			}
		}

		if (equByte(E,F) && equByte(E,B)) {
			if (!equByte(E,D) && tsign(E-D) > 0 && tsign(F-F1) > 0) {
				*negfig = 1;
				return 0;
			} else if (!equByte(E,D) && tsign(E-D) < 0 && tsign(F-F1) < 0) {
				*negfig = 0;
				return 0;
			} else if (!equByte(E,H) && tsign(E-H) > 0 && tsign(B-B1) > 0) {
				*negfig = 1;
				return 0;
			} else if (!equByte(E,H) && tsign(E-H) < 0 && tsign(B-B1) < 0) {
				*negfig = 0;
				return 0;
			} else {
				return 1;
			}
		}

		if (equByte(E,F) && equByte(E,H)) {
			if (!equByte(E,D) && tsign(E-D) > 0 && tsign(F-F1) > 0) {
				*negfig = 1;
				return 0;
			} else if (!equByte(E,D) && tsign(E-D) < 0 && tsign(F-F1) < 0) {
				*negfig = 0;
				return 0;
			} else if (!equByte(E,B) && tsign(E-B) > 0 && tsign(H-H1) > 0) {
				*negfig = 1;
				return 0;
			} else if (!equByte(E,B) && tsign(E-B) < 0 && tsign(H-H1) < 0) {
				*negfig = 0;
				return 0;
			} else {
				return 1;
			}
		}

		if (equByte(E,D) && equByte(E,H) && m != CE) {
			if (!equByte(E,F) && tsign(E-F) > 0 && tsign(D-D1) > 0) {
				*negfig = 1;
				return 0;
			} else if (!equByte(E,F) && tsign(E-F) < 0 && tsign(D-D1) < 0) {
				*negfig = 0;
				return 0;
			} else if (!equByte(E,B) && tsign(E-B) > 0 && tsign(H-H1) > 0) {
				*negfig = 1;
				return 0;
			} else if (!equByte(E,B) && tsign(E-B) < 0 &&  tsign(H-H1) < 0) {
				*negfig = 0;
				return 0;
			} else {
				return 1;
			}
		}
		return 0;
	}

	/* Функция определения геометрии. */
	int identify(
		     int dir,
		     int opposite,
		     unsigned char ortvalue1,
		     unsigned char ortvalue2,
		     unsigned char oppvalue,
		     int thinline,
		     int dirside,
		     int oppositeside,
		     int dircorner,
		     int oppositecorner
	)
	{

	  if (equ(dir, opposite)) {
	    /* Полярность одиночной линии обратна полярности
	     * градиента. */
	    *negfig = (V[m] > 0);
	    *inverse = *negfig;
	    return thinline;
	  } else if (bGZ(ortvalue1, ortvalue2)) {
	    /* Острый угол всегда печатный элемент. */
	    *negfig = 0;
	    *inverse = 0;
	    if (V[m] > 0) {
	      return dircorner;
	    } else {
	      return oppositecorner;
	    }
	  } else if (bLZ(ortvalue1,ortvalue2)) {
	    /* Вогнутый угол всегда пробельный элемент. */
	    *negfig = 1;
	    *inverse = 1;
	    if (V[m] > 0) {
	      return oppositecorner;
	    } else {
	      return dircorner;
	    }
	  } else {
	    *negfig = (V[m] < 0) && oppvalue <= 128;
	    *inverse = *negfig;
	    if (V[m] < 0) {
	      if (*negfig) {
		return dirside;
	      } else {
		return oppositeside;
	      }
	    } else {
	      if (*negfig) {
		return oppositeside;
	      } else {
		return dirside;
	      }
	    }
	  }
	}

	/* Основной алгоритм анализа. */

	/* Вычисление суммарных значений отсчётов по 8-ми базовым
	 * направлениям. */

	ks = (double)(3-FDcor)/2;
	V[0] = D + E + F - A - B - C;
	V[1] = A + E + I - B*ks - C*FDcor - F*ks;
	V[2] = B + E + H - C - F - I;
	V[3] = C + E + G - F*ks - I*FDcor - H*ks;
	V[4] = D + E + F - G - H - I;
	V[5] = A + E + I - D*ks - G*FDcor - H*ks;
	V[6] = B + E + H - A - D - G;
	V[7] = C + E + G - B*ks - A*FDcor - D*ks;

	/* Нахождение наибольшей по модулю суммы. */
	m = amax(V);

	/* Если установлен признак проверки на пересечение центрального
	 * участка, производится данная проверка и, в случае получения
	 * отрицательного результата, в качестве номера тайла возвращается
	 * 0, обозначающий стационартый участок. */
	if (outtest && COut(m)) {
	  return 0;
	}

	/* Направление, в котором был найден наиболее ярко выраженный перепад
	 * задаёт направление пересечения границы контура, а результат
	 * дополнительного сравнения окрестных значений определяет
	 * геометрию контура. */
	  switch  (m) {
		  case FE:
		    return identify(
				    FE, DE, B, H, D,
			     TILE_NL, TILE_ES, TILE_WS, TILE_EC, TILE_WC
		    );

		  case HE:
		    return identify(
				    HE, BE, D, F, B,
			     TILE_WL, TILE_SS, TILE_NS, TILE_SC, TILE_NC
		    );

		  case IE:
		    return identify(
				    IE, AE, C, G, A,
			     TILE_NEL, TILE_SES, TILE_NWS, TILE_SEC, TILE_NWC
		    );
  
		  case GE:
		    return identify(
				    GE, CE, A, I, C,
			     TILE_NWL, TILE_SWS, TILE_NES, TILE_SWC, TILE_NEC
		    );

		  case DE:
		    return identify(
				    DE, FE, B, H, F,
			     TILE_NL, TILE_WS, TILE_ES, TILE_WC, TILE_EC
		    );

		  case AE:
		    return identify(
				    AE, IE, G, C, I,
			     TILE_NEL, TILE_NWS, TILE_SES, TILE_NWC, TILE_SEC
		    );

		  case BE:
		    return identify(
				    BE, HE, D, F, H,
			     TILE_WL, TILE_NS, TILE_SS, TILE_NC, TILE_SC
		    );

		  case CE:
		    return identify(
				    CE, GE, A, I, G,
			     TILE_NWL, TILE_NES, TILE_SWS, TILE_NEC, TILE_SWC
		    );

		  default :
			  return 0;
	  }
}

/* Определяет, является ли тайл с указанными параметрами слишком тонким
 * по сравнению с определённым пределом. */
int too_thin(int tile_index, unsigned char tile_area) {

	return tile_area < minarea &&
		(tile_index == TILE_NL ||
		 tile_index == TILE_WL ||
		 tile_index == TILE_NWL ||
		 tile_index == TILE_NEL ||
		 tile_index == TILE_WS ||
		 tile_index == TILE_NS ||
		 tile_index == TILE_ES ||
		 tile_index == TILE_SS);
}

/* Определение геометрии участка и разделение его значения. Возвращает
 * номер тайла, площадь штрихового элемента, фоновое значение тона и
 * признак вычитания тайла из фона (инферсии). */
void get_tile(t_window window, int *neg, int *tile_index, unsigned char *tile_area, unsigned char *bg_value) {
	
	/* Хранит номер тайла. */
	unsigned char index;

	/* Признак инверсии. */
	int inverse;
	
	/* Вычисление относительной площади тайла. */
	void get_area() {

		/* Хранит экстремальное значение в окрестности. */
		unsigned char m;

		/* Выбор в качестве значения фона максимального или минимального
		 * значения тона в зависимости от полярности тайла. Расчёт
		 * площади тайла. */
		if (!inverse) {
		  m = max(window);
		  *bg_value = m;
		  *tile_area = (unsigned char) rint( 255.0 * ( (double)((255-E) - (255-m)) / (255 - (255-m)) ) );
		} else {
		  m = min(window);
		  *bg_value = m;
		  *tile_area = (unsigned char) rint( 255.0 * ( (double)((255-m) - (255-E)) / (255-m) ) ) ;
		}
	}

	/* Получение номера тайла и его полярности через анализ окрестных
	 * значений. */
	index = get_tile_index(window, neg, &inverse);
	
	/* Если номер тайла не соответствует пробельному участку, то
	 * производится вычисление его площади и фонового значения. Далее
	 * это значение корректируется с учётом установленных ограничений. */
	if (index) {
		get_area();
		/* Установка кода пробельного участка в качестве номера тайла,
		 * если площадь штриха меньше установленного порога. */
		if (*tile_area == 0 || too_thin(*tile_index, *tile_area)) {
		  *tile_index = 0;
		  *bg_value = E;
		} else {
		  *tile_index = index;
		}
	} else {
		/* Установка значения тона для пробельного участка. */
		*tile_index = 0;
		*tile_area = 0;
		*bg_value = E;
	}
}

/* Инициализация набора счётчиков пробельных участков. */
void init_maketiles_info(struct maketiles_info *mi) {
	
	mi->pz = 0;
	mi->nz = 0;
	mi->pzl = 0;
	mi->nzl = 0;

}

