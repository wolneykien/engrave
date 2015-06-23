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


/* ���������� ������� ��� ������� ���� ���ޣ���. */

#include "tile32f.h"
#include <stdio.h>
#include <math.h>

/* ��������� ��������� �������. */

/* ������� �������� ����������� ������� ����������� ������� �����������. */
int outtest = 1;

/* ����� ��������� ��������� ���ޣ��� (0 - 255). */
unsigned char FThr = 13;

/* ����� ��������� ��������� �������� ���ޣ��� (0 - 3*255). */
double FThr2 = 38.4;

/* ����������� ���������� ��� ����������� ������������� ���ޣ���. */
double FDcor = 1.0;

/* ����������� ������� ������. */
unsigned char minarea = 1;

/* �������� ������� ����������. */

/* ������������ ������� ���ޣ��� ������ �����������, ������������� �
 * ��������� ������ � ��������� �� ���ޣ��� ���������� ������� � ����������
 * ���������� ����� ���ޣ���. */
void edgecpy(char *buf, size_t width, size_t ss) {

	memcpy(buf, buf+2*ss, ss);
	memcpy(buf+ss, buf, ss);
	memcpy(buf+2*ss+width*ss, buf+2*ss+width*ss-ss, ss);
	memcpy(buf+2*ss+width*ss+ss, buf+2*ss+width*ss, ss);

}

/* ������� ��� ������ ���������� �� ������ �����. */
int
amax(double *V)
{
	int i;
	double m;
	int res;

	/* ��������� �������� ��� ��������� ����. */
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

/* ��������������� �������: ����� ������������� ��������
 * � �����������. */
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
		
/* ��������������� �������: ����� ������������ ��������
 * � �����������. */
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

/* ������ ����������� � ���������� ������ � ������������� �������
 * �����. ����� ���� ���ޣ���, ����������� ������� ��������. */
int get_tile_index(t_window window, int *negfig, int *inverse) {

	/* ��������� �������� ���ޣ��� �� 8-�� ������������. */
	double V[8];

	/* ������ ���������� �� ������ �����. */
	int m;

	/* ����������� ���������� ��� ���ޣ��� � ����. */
	double ks;

	/* ��������������� �������. */

	/* ������� ��������� ��������� ��������. ����������� �������
	 * �������������� ����������� ��������� ��������. */
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

	/* ����������, ��������� �� ����������� ���������
	 *  ����������.
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

	/* ������� ������ ����, ���� ����������� ��������� ����������
	 *  ���������, ����� -- ������.
	 *//*
	int codir(int i1, int i2, int t1, int t2) {
	  if (iscodir(i1, i2)) {
	    return t1;
	  } else {
	    return t2;
	  }
	  }*/

	/* ����������� ����, ��� ��� ��������� �������� �����
	 * ���������� ���������� � ����������� ��������� ���������
	 * �����. */
	int bGZ(unsigned char s1, unsigned char s2) {
		return ((s1-E) > FThr && (s2-E) > FThr);
	}

	/* ����������� ����, ��� ��� �������� ����� ����������
	 * ���������� � ����������� ��������� ������
	 * �������������� �������� ������. */
	int bLZ(unsigned char s1, unsigned char s2) {
		return ( (s1-E) < -FThr && (s2-E) < -FThr);
	}

	/* ��������� ������� ��������� ���ޣ��� � �ޣ��� ������. */
	int equByte(unsigned char s1, unsigned char s2) {
		return (abs(s1-s2) < FThr);
	}

	/* ���������� ���� ��������� � �ޣ��� ������. */
	int tsign(int dif) {
	  if (dif < -FThr)
	    return -1;
	  else if (dif > FThr)
	    return 1;
	  else
	    return 0;
	}

	/* �������� ����������� �������� ������������ ������� �����
	 * �����������. */
	int COut(int m) {
		/* ������ ����� �������� �����������. */
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

	/* ������� ����������� ���������. */
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
	    /* ���������� ��������� ����� ������� ����������
	     * ���������. */
	    *negfig = (V[m] > 0);
	    *inverse = *negfig;
	    return thinline;
	  } else if (bGZ(ortvalue1, ortvalue2)) {
	    /* ������ ���� ������ �������� �������. */
	    *negfig = 0;
	    *inverse = 0;
	    if (V[m] > 0) {
	      return dircorner;
	    } else {
	      return oppositecorner;
	    }
	  } else if (bLZ(ortvalue1,ortvalue2)) {
	    /* �������� ���� ������ ���������� �������. */
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

	/* �������� �������� �������. */

	/* ���������� ��������� �������� ���ޣ��� �� 8-�� �������
	 * ������������. */

	ks = (double)(3-FDcor)/2;
	V[0] = D + E + F - A - B - C;
	V[1] = A + E + I - B*ks - C*FDcor - F*ks;
	V[2] = B + E + H - C - F - I;
	V[3] = C + E + G - F*ks - I*FDcor - H*ks;
	V[4] = D + E + F - G - H - I;
	V[5] = A + E + I - D*ks - G*FDcor - H*ks;
	V[6] = B + E + H - A - D - G;
	V[7] = C + E + G - B*ks - A*FDcor - D*ks;

	/* ���������� ���������� �� ������ �����. */
	m = amax(V);

	/* ���� ���������� ������� �������� �� ����������� ������������
	 * �������, ������������ ������ �������� �, � ������ ���������
	 * �������������� ����������, � �������� ������ ����� ������������
	 * 0, ������������ ������������ �������. */
	if (outtest && COut(m)) {
	  return 0;
	}

	/* �����������, � ������� ��� ������ �������� ���� ���������� �������
	 * ������ ����������� ����������� ������� �������, � ���������
	 * ��������������� ��������� ��������� �������� ����������
	 * ��������� �������. */
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

/* ����������, �������� �� ���� � ���������� ����������� ������� ������
 * �� ��������� � ������̣���� ��������. */
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

/* ����������� ��������� ������� � ���������� ��� ��������. ����������
 * ����� �����, ������� ���������� ��������, ������� �������� ���� �
 * ������� ��������� ����� �� ���� (��������). */
void get_tile(t_window window, int *neg, int *tile_index, unsigned char *tile_area, unsigned char *bg_value) {
	
	/* ������ ����� �����. */
	unsigned char index;

	/* ������� ��������. */
	int inverse;
	
	/* ���������� ������������� ������� �����. */
	void get_area() {

		/* ������ ������������� �������� � �����������. */
		unsigned char m;

		/* ����� � �������� �������� ���� ������������� ��� ������������
		 * �������� ���� � ����������� �� ���������� �����. ���ޣ�
		 * ������� �����. */
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

	/* ��������� ������ ����� � ��� ���������� ����� ������ ���������
	 * ��������. */
	index = get_tile_index(window, neg, &inverse);
	
	/* ���� ����� ����� �� ������������� ����������� �������, ��
	 * ������������ ���������� ��� ������� � �������� ��������. �����
	 * ��� �������� �������������� � �ޣ��� ������������� �����������. */
	if (index) {
		get_area();
		/* ��������� ���� ����������� ������� � �������� ������ �����,
		 * ���� ������� ������ ������ �������������� ������. */
		if (*tile_area == 0 || too_thin(*tile_index, *tile_area)) {
		  *tile_index = 0;
		  *bg_value = E;
		} else {
		  *tile_index = index;
		}
	} else {
		/* ��������� �������� ���� ��� ����������� �������. */
		*tile_index = 0;
		*tile_area = 0;
		*bg_value = E;
	}
}

/* ������������� ������ �ޣ������ ���������� ��������. */
void init_maketiles_info(struct maketiles_info *mi) {
	
	mi->pz = 0;
	mi->nz = 0;
	mi->pzl = 0;
	mi->nzl = 0;

}

