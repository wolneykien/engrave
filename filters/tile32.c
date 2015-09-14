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


/*
 * ������ tile32 ���������� PostScrpit-��� ��� ��������� 32 ��������� ��
 * ��������� ������ ��� �������� ��������� ����� �����������, ������������� ��
 * ����������� ����.
*/

#include <stdio.h>
#include <sys/types.h>
#include <string.h>

#include "system.h"
#include "filter.h"
#include "tile32f.h"
#include "misc.h"

/* ��� ����������, ��������������� �� ������. */

#define EXIT_FAILURE 1

/* ���� �������� ������� ������� ���������� � ������� �������. */
enum {DUMMY_CODE=129
};

/* �������� ���������� */

/* ����������� ��������� ����������: */
int want_half = 0;		/* ������������ ������ (�����)
				 * ��������
				 * �����������; */
char *histfn;			/* ��� ����� ��� ������ �����������
				 * ������������� �������; */

/* ���������� ��� ������� ���������� ������������� ������: */
int phist[TILE_COUNT];          /* ����������� ���������� ������; */
int nhist[TILE_COUNT];          /* ����������� ���������� ������; */
int zerotile = 0;               /* ���������� ������������
				 * ��������. */

/* ��������� �������� ���������� ��������� �������. */

/* ����� ��������� ��������� ���ޣ��� (0 - 255). */
char *FThr_str;

/* ����� ��������� ��������� �������� ���ޣ��� (0 - 3*255). */
char *FThr2_str;

/* ����������� ���������� ��� ����������� ������������� ���ޣ���. */
char *FDcor_str;

/* ����������� ������� ������. */
char *minarea_str;

/* ��������� ���������� ��������� �������� �������� �������. */
char *passthrough_str;
int passthrough[] = {0, 0, 0, 0};

/* ��������� ���������� ��������� �����. */
char *select_mask_str;
int select_mask[] = {1, 1};

/* ����������� ���������� ��������� ������. */
static struct option const long_options[] =
{
	{"half", no_argument, &want_half, 1},
	{"hist", required_argument, NULL, 0},
	{"ignore-outtest", no_argument, &outtest, 0},
	{"minarea", required_argument, NULL, 0},
	{"value-thr", required_argument, NULL, 0},
	{"sum-thr", required_argument, NULL, 0},
	{"dia-corr", required_argument, NULL, 0},
	{"passthrough", required_argument, NULL, 0},
	{"select-mask", required_argument, NULL, 0},
	{NULL, 0, NULL, 0}
};

/* ��������� �� ����������, ��������������� ���������� ���������� ������. */
char **option_vars[] =
{
	NULL, &histfn, NULL, &minarea_str,
	&FThr_str, &FThr2_str, &FDcor_str, &passthrough_str,
	&select_mask_str
};

/* ����������� ��������������� �������. */

/* ����� ��������� PostScript-��������� � ��������� �����, � ������������
 * �������� �������� ����������� �����.
 */
void write_header(FILE *stream, int mask) {
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
			mask ? "0.0" : "1.0", mask ? "negative" : "positive",
			(float) height/hres*72,
			(float) 72/hres,
			(float) 72/vres);
  }
}

/* ����� ����������� ����� PostScript-��������� � ��������� �����. */
void write_footer(FILE *stream) {
  if (stream != NULL) {
	fprintf(stream, "%%%%EndData\n"
			"grestore\t%% Restore previous graphic state\n");
  }
}

/* �������� �������, ���������� ��� ����������� �������������: ������
 * ����������� �����, ���������� ������� � ������������ ������� � ���������
 * ����� ����������� � ���� ������. ������ ����������� ������������� �
 * ��������� ������ �������, ������ � ������������� ������� ������ ������������
 * � ��������� �������� �����. ���������� ���������� ��������� ������
 * ���������� �� ���ޣ��� �� ��������� ���������� ��������. ������ �����������
 * ������� �� ���ޣ��� ��������� ����� � �� ���������� ���������� �����
 * ���ޣ���. ���������� � ������ (������ � �������) ������� ����������� �
 * ���� ������ ��������, ��� ��������� � PostScript-���������. ���������� �
 * ���������� ����� ���������� �� �����������, ��������� ��������� � ���������
 * ������ ���������. ��� �� ����������� ������������ ��� ��������� ���������.
 * ��� ������������� �ޣ������ ������ �������� � ������ ����� ����� ��������
 * �������, ��� �������� � ��������� �� ���������� ������.
 */
void maketiles(unsigned char *buf[], unsigned char *outbuf, \
	       int c0, int c, size_t ss, size_t len, \
	       struct ascii85 *pos_ascii85_p, \
	       struct ascii85 *neg_ascii85_p, \
	       struct maketiles_info *mi) {

	/* ���� ���ޣ���. */
	t_window window;

	/* �ޣ���� */
	int x;

	/* ������� ����������. */
	int neg;

	/* ����� �����. */
	int tile_index;

	/* ������������� ������� �����. */
	unsigned char tile_area;

	/* ������� ���� �ޣ������ ��� ������ �������� � ������ �����. */
	unsigned int z;
	unsigned int zl;

	/* ������� ��������� ��� ����������� ���������� � ������. */
	struct ascii85 *ascii85_p;

	/* ������ �������� �����������. */
	size_t half_len = len/2;

	/* �������� ��������� ������ */
	size_t c_offs = c - c0;

	/* ����� �ޣ������ ������ �������� ��� ����������� � �����������
	 * ��������� �����������.
	 */
	mi->pz = 0;
	mi->nz = 0;

	/* ������������� ���������� �� ���ޣ�� � ���� � �ޣ��� �������
	 * ���ޣ�� � ������ � �������� ��������� ������.
	 *
	 * ��� ������� � ����������, ���������� � ��������� window ������������
	 * ���������� ����������������.
	 */
	pB1 = buf[0] + 2*ss + c_offs;
	pA = buf[1] + ss + c_offs;
	pB = buf[1] + 2*ss + c_offs;
	pC = buf[1] + 3*ss + c_offs;
	pD1 = buf[2] + c_offs;
	pD = buf[2] + ss + c_offs;
	pE = buf[2] + 2*ss + c_offs;
	pF = buf[2] + 3*ss + c_offs;
	pF1 = buf[2] + 4*ss + c_offs;
	pG = buf[3] + ss + c_offs;
	pH = buf[3] + 2*ss + c_offs;
	pI = buf[3] + 3*ss + c_offs;
	pH1 = buf[4] + 2*ss + c_offs;

	/* ��������� ������� ������ � �������� ������ � �ޣ��� ��������
	 * ��������� ������
	 */
	outbuf += c_offs;

	/* ������������ ����� ����������� ����� ���ޣ���. */
	for (x = 0; x < len; x++) {
		/* ���� ���������� ����������� ������������� ���������� ������
		 * ��������� �����������, �� ����� ����������� ������� ������
		 * ����������� ����� �� ������������ � �������� �����.
		 * �������������� �������� �������� ������� ��������
		 * ��������� ������.
		 */
	        if ((want_half && x > half_len)
		    || passthrough[c]) {
			tile_index = 0;
			*outbuf = E;
		} else {
			/* ������ ��������� ������� � ����������� ������ �����
			 * � ��� ������������� �������, ������ �������� ��������
			 * ���� � �������� �����.
			 */
			get_tile(window, &neg, &tile_index, &tile_area, outbuf);
		}

		/* �������� ���� � ������� ������ ������. */
		pA += ss;
		pB += ss;
		pC += ss;
		pD += ss;
		pE += ss;
		pF += ss;
		pG += ss;
		pH += ss;
		pI += ss;
		pB1 += ss;
		pD1 += ss;
		pH1 += ss;
		pF1 += ss;
		outbuf += ss;

		/* � ������, ���� ��� ��������������� ��������� �������,
		 * ������������ ������ ���������� � Σ� � PostScript-���.
		 */
		if (tile_index) {
			/* ��������� ���������� � ����� ������������ �
			 * ����������� �� ��� ����������.
			 */
                        if (neg) {
				/* ����������� ������ ����� � �����������
				 * �����.
				 */
				nhist[tile_index-1]++;
				/* ��������� �������� ������� ����������
				 * ��� ��������� ���������� �����. */
				ascii85_p = neg_ascii85_p;
				z = mi->nz;
				zl = mi->nzl;
			} else {
				/* ����������� ������ ����� � �����������
				 * �������.
				 */
				phist[tile_index-1]++;
				/* ��������� �������� ������� ����������
				 * ��� ��������� ���������� �����. */
				ascii85_p = pos_ascii85_p;
				z = mi->pz;
				zl = mi->pzl;
			}


			/* ����� ������� ������ � ������� �����, ������������
			 * ���������� � �������������� � ������������ ��������,
			 * � ������������ �� ���������� �ޣ������.
			 */
			if (zl) {
				ascii85_encode(ascii85_p, 0xFF);
				ascii85_encode(ascii85_p, 0xFF);
				ascii85_encode(ascii85_p, (unsigned char)((zl >> 8) & 0xFF));
				ascii85_encode(ascii85_p, (unsigned char)(zl & 0xFF));
			}
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
				
			/* ������������ ����� � ������� �����. */
			ascii85_encode(ascii85_p, (unsigned char)tile_index);			
			ascii85_encode(ascii85_p, tile_area);
				
			/* ���������� � ����� �ޣ������ ������������
			 * ��������. */
			if (neg) {
				mi->nz = 0;
				mi->nzl = 0;
				mi->pz++;
			} else {
				mi->pz = 0;
				mi->pzl = 0;
				mi->nz++;
			}
		} else {
			/* �� ������������ ������� ����������� ������ 
			 * ����������� �ޣ����� ��������. */
			mi->nz++;
			mi->pz++;
                        zerotile++;
		}
	}


	/* ����������� �ޣ����� ������ (������������) �����. */
	mi->nzl++;
	mi->pzl++;

}

/* ����� ��������� ������� �������. */
void
usage_header (FILE *out)
{
  fprintf (out, _("%s - \
Image filter for 'engrave' package. Produces PostScript code for the stroke part of each image color channel reading a RAW CT image data from stdin\n"), program_name);
  
}

/* ����� ������� ������� �� ����������� ���������. */
void
usage_params(FILE *out)
{

  fprintf(out, _("\
  --half	       process only the left half of an image\n\
  --hist=FILE	       write tile histogram data to FILE\n\
  --ignore-outtest     do not test stroke to cross the centra area\n\
  --minarea=VALUE      minimal line stroke area (defines thickness),\n\
		       default is 1\n\
  --value-thr=VALUE    threshold (0 - 255) to compare values,\n\
		       default is 13 (5%)\n\
  --sum-thr=VALUE      threshold (0 - 3*255) to compare summary\n\
		       values, default is 38.4\n\
  --dia-corr=VALUE     correlation coefficient of the diagonal and\n\
		       orthogonal sampling values\n\
  --passthrough=CMYK   ignores selected color channels\n\
  --select-mask=BW     select black, white or both correction\n\
                       images\n\
"));

}

/* ��������� ���������� ��������� � ������������ � ����������� ����������
 * ������. */
void
set_paramenetrs()
{

	/* ��������������� ���������. */
	char *endptr;

	/* ������ �� ��������� �������� ���������� ������������� � ��������
	 * �����. */

	if (FThr_str != NULL) {
		FThr = (char) strtol(FThr_str, &endptr, 0);
		if (endptr == FThr_str) {
			fprintf(stderr, "Value comparison threshold should be an integer number.\n");
	  		/* ����� � ��������� ������, ���� ������� �������������
	   	 	 * ����������� ��������. */
	  		exit(EXIT_FAILURE);
		}
	}

	if (FThr2_str != NULL) {
		FThr = strtod(FThr2_str, &endptr);
		if (endptr == FThr2_str) {
			fprintf(stderr, "Summary values comparison threshold should be a decimal number.\n");
	  		/* ����� � ��������� ������, ���� ������� �������������
	   	 	 * ����������� ��������. */
	  		exit(EXIT_FAILURE);
		}
	}

	if (FDcor_str != NULL) {
		FThr = strtod(FDcor_str, &endptr);
		if (endptr == FDcor_str) {
			fprintf(stderr, "Diagonal correlator should be a decimal number.\n");
	  		/* ����� � ��������� ������, ���� ������� �������������
	   	 	 * ����������� ��������. */
	  		exit(EXIT_FAILURE);
		}
	}

	if (minarea_str != NULL) {
		minarea = (char) strtol(minarea_str, &endptr, 0);
		if (endptr == minarea_str) {
			fprintf(stderr, "Minimal line area should be a decimal number.\n");
	  		/* ����� � ��������� ������, ���� ������� �������������
	   	 	 * ����������� ��������. */
	  		exit(EXIT_FAILURE);
		}
	}

	if (passthrough_str != NULL) {
	  if (strchr(passthrough_str, 'C') != NULL) {
	    passthrough[0] = 1;
	  }
	  if (strchr(passthrough_str, 'M') != NULL) {
	    passthrough[1] = 1;
	  }
	  if (strchr(passthrough_str, 'Y') != NULL) {
	    passthrough[2] = 1;
	  }
	  if (strchr(passthrough_str, 'K') != NULL) {
	    passthrough[3] = 1;
	  }
	}
	if (select_mask_str != NULL) {
	  if (strchr(select_mask_str, 'B') != NULL) {
	    select_mask[0] = 1;
	  } else {
	    select_mask[0] = 0;
	  }
	  if (strchr(select_mask_str, 'W') != NULL) {
	    select_mask[1] = 1;
	  } else {
	    select_mask[1] = 0;
	  }
	}
}

/* �������� �������. */
int
main (int argc, char **argv)
{

  /* ����� ������� ��������������� ���������. */
  int opt_r;

  /* ������ ���ޣ�� � ������. */
  size_t ss;
   /* ���� ��� ������ �����������. */
  FILE *histf = NULL;

  /* ����� ���������� �� ����� ��� ������ ���������� ��������� �����������. */
  FILE *pos_outfile[] = {
	  NULL,
	  NULL,
	  NULL,
	  NULL
  };

  /* ����� ������ ��� ������ ���������� ��������� �����������. */
  char pos_filenames[4][MAXLINE] = { "\0", "\0", "\0", "\0" };
  
  /* ��������� ������������ ��� ���������� ���������� � ���������� ������. */
  struct ascii85 *pos_ascii85_p[] = {
	  NULL,
	  NULL,
	  NULL,
	  NULL
  };
  
  /* ����� ���������� �� ����� ��� ������ ����������� �����������. */
  FILE *neg_outfile[] = {
	  NULL,
	  NULL,
	  NULL,
	  NULL
  };
  
  /* ����� ������ ��� ������ ����������� �����������. */
  char neg_filenames[4][MAXLINE] = { "\0", "\0", "\0", "\0" };

  /* ��������� ������������ ��� ���������� ���������� � ����������� ������. */
  struct ascii85 *neg_ascii85_p[] = {
	  NULL,
	  NULL,
	  NULL,
	  NULL
  };

  /* ����� ������� ��� �������� 5 ����� �����������, ������� �����
   * ��������������� ����������� �����.
   */
  unsigned char *buf[] = {
	  NULL,
	  NULL,
	  NULL,
	  NULL,
	  NULL
  };

  /* ��������� ��� ������������� �������� �ޣ������ ����� �������� �������
   * ��������� ������.
   */
  struct maketiles_info mi[4];
  
  /* ��������������� ���������� ��� ����������� ����������. */
  unsigned char *tmpbuf = NULL;
  
  /* ����� ��� �������� ������� �������� ����, ������������ �� ����������
   * ���������.
   */
  unsigned char *outbuf = NULL;	/* Buffer for output image data */ 
  
  /* �ޣ���� ��� �������� �������. */
  int c0, c, cN;

  /* ����������� �ޣ���� ��� �������� ����� ������. */
  size_t rd;

  /* ����� ������ �����������. */
  int y;

  /* �ޣ���� ��� ������������� ��������. */
  int i;

  /* ������� ��������� ���������� ���������. */
  int OK = 0;

  /* ������� ������� ������� ��������. ������������ ������� ������ � ��������
   * ��������� ������, ���� ��������� ����������� ��������. � ��������� ������,
   * ��������� ����� ������� �������� ���������, ����� �� ��������� � ��������
   * PostScript-����.
   */
  void cleanup() {
	  int i;

	  if (!OK)
		  fprintf(stderr, "%s: Finished with error.\n", program_name);
	  for (i = 0; i < 4; i++) {
		  if (pos_ascii85_p[i] != NULL)
			  destroy_ascii85(pos_ascii85_p[i]);
		  if (pos_outfile[i] != NULL)
			  fclose(pos_outfile[i]);
		  if (!OK && pos_filenames[i][0] != '\0') {
			  fprintf(stderr, "%s: Delete temporary file: %s\n", program_name, pos_filenames[i]);
			  unlink(pos_filenames[i]);
		  }
		  if (neg_ascii85_p[i] != NULL)
			  destroy_ascii85(neg_ascii85_p[i]);
		  if (neg_outfile[i] != NULL)
			  fclose(neg_outfile[i]);
		  if (!OK && neg_filenames[i][0] != '\0') {
			  fprintf(stderr, "%s: Delete temporary file: %s\n", program_name, neg_filenames[i]);
			  unlink(neg_filenames[i]);
		  }
	  }
	  
	  for (i = 0; i < 5; i++)
		  if (buf[i] != NULL)
			  free(buf[i]);
	  if (outbuf != NULL)
		  free(outbuf);
  }

  /* ��������� ����� ��������. */
  program_name = argv[0];

  /* ������������� ������� ��������� �������. */
  init_cleanup(program_name);
  push_cleanup(cleanup);

  /* ������ ��������� ��������� ������. */
  opt_r = decode_switches (argc, argv, EXIT_FAILURE, long_options, option_vars, &usage_header, &usage_params);

  /* �������������� ��������� �������� ����������. */
  set_paramenetrs();

  /* �������� �������� ���� ����������� ���������� �����������. */
  if (!width || !height || !hres || !vres) {
	  fprintf(stderr, "%s: WIDTH, HEIGHT, HRES & VRES should be specified.\n", program_name);
	  /* ����� � ��������� ������, ���� �� ��� ��������� �����������
	   * ���� �������.
	   */
	  exit(EXIT_FAILURE);
  }
	 
  /* ��������� ������� ���ޣ�� � �ޣ������ ������� � ������������ � ���������
   * ����� �����������.
   */
  if (is_cmyk) {
	  ss = 4;	/* 4 ����� ��� 4-���������� �����������; */
	  c0 = 0;	/* 4 �����, � 0 */
	  cN = 3;	/* �� 3 */
  } else {
	  ss = 1;	/* 1 ���� ��� ����������� � ����� �����; */
	  c0 = 3;	/* 1 ����, � 0 */
	  cN = 3;	/* �� 0 */
  }

  /* ��������� ������ ��� ������. �������������� 2 ���ޣ�� � ������ �������
   * ������ ������������ � ����� ���������� ������� ��������. */
  for (i = 0; i < 5; i++) {
	  buf[i] = calloc(ss, width + 4);
	  if (buf[i] == NULL) {
		  fprintf(stderr, "%s: Scanline buffer #%i allocation failed\n", program_name, i);
		  /* ����� � ��������� ������, ���� ������� ��������� ������,
		   * ����������� ��������. */
		  exit(EXIT_FAILURE);
	  }
  }

  /* ��������� ������ ��� �������� �����. */
  outbuf = calloc(ss, width);
  if (outbuf == NULL) {
	  fprintf(stderr, "%s: Output buffer allocation failed\n", program_name);
	  /* ����� � ��������� ������, ���� ������� ��������� ������,
	   * ����������� ��������. */
	  exit(EXIT_FAILURE);
  }

  /* ������������� ���������. */
  for (i = 0; i < TILE_COUNT-1; i++) {
	  phist[i] = 0;
	  nhist[i] = 0;
  }

  /* �������� ��������� ������ � ������������� ������������. */
  for (c = c0; c <= cN; c++) {
	 /* �������� ������. */
    if (select_mask[0]) {
      pos_outfile[c] = open_tmp_file("s", c, pos_filenames[c]);
    }
    if (select_mask[1]) {
      neg_outfile[c] = open_tmp_file("m", c, neg_filenames[c]);
    }
	 /* ����� ����������. */
	 write_header(pos_outfile[c], 0);
	 write_header(neg_outfile[c], 1);
	 
	 /* ������������� ������������. */
	 pos_ascii85_p[c] = new_ascii85(pos_outfile[c]);
	 neg_ascii85_p[c] = new_ascii85(neg_outfile[c]);
	 if (pos_ascii85_p[c] == NULL || neg_ascii85_p[c] == NULL) {
		 fprintf(stderr, "%s: Failed to allocate an ascii85 structure.\n", program_name);
		 /* ����� � ��������� ������, ���� ������������� �����������
		  * �������.
		  */
		 exit(EXIT_FAILURE);
	 }
  }

  /* ����� ��������� � ������� ����������. */
  if (want_verbose) {
    if (histfn != NULL) {
      fprintf(stderr, "[%s] Histogram file: %s\n", program_name, histfn);
    }
    fprintf(stderr, "[%s] Byte threshold: %u\n", program_name, FThr);
    fprintf(stderr, "[%s] Summ threshold: %.2f\n", program_name, FThr2);
    fprintf(stderr, "[%s] Diagonal correlator: %.2f\n", program_name, FDcor);
    fprintf(stderr, "[%s] Minimum line area: %u\n", program_name, minarea);
    fprintf(stderr, "[%s] Middle area test: %s\n", program_name, outtest ? "on" : "off");
    if (passthrough_str != NULL) {
      fprintf(stderr, "[%s] Passthrough separations: %s\n", program_name, passthrough_str);
    }
    if (select_mask_str != NULL) {
      fprintf(stderr, "[%s] Selected correction images: %s\n", program_name, select_mask_str);
    }
  }

  /* ������ � ��������� ����� �����������. */

  /* ��������������� ������ ������ ����������� � ����� 4-�� ������ (3), �
   * ������� 3-�� ���ޣ�� (2). */
  rd = freadsmp(buf[3]+2*ss, ss, width, stdin, miniswhite);
  /* ���� ���������� ����������� ���ޣ��� ��������� ������ ����� ������
   * �����������, �� ������������ ����� � ��������� ������. */
  if (rd < width) {
	  fprintf(stderr, "%s: Line 0. Image stream suddenly closed (%i samples has been read)\n", program_name, rd);
	  exit(EXIT_FAILURE);
  }
  /* ����������� ������� ���ޣ��� 4-�� ������ (3) ��� ����������� �������
   * ��������. */
  edgecpy(buf[3], width, ss);
 
  /* ���� � ����������� ������ ����� ������, �� ������������ ���������������
   * ������ ������ ����������� � ����� 5-�� ������ (4), � ������� 3-�� ���ޣ��
   * (2). */
  if (height > 1) {
	  rd = freadsmp(buf[4]+2*ss, ss, width, stdin, miniswhite);
	  /* ���� ���������� ����������� ���ޣ��� ��������� ������ ����� ������
	   * �����������, �� ������������ ����� � ��������� ������. */
	  if (rd < width) {
		  fprintf(stderr, "%s: Line 1. Image stream suddenly closed (%i samples has been read)\n", program_name, rd);
		  exit(EXIT_FAILURE);
	  } 
  }
  /* ����������� ������� ���ޣ��� 5-�� ������ (4) ��� ����������� �������
   * ��������. */
  edgecpy(buf[4], width, ss);

  /* ����������� 4-�� ������ (3) ������: � 3-� (2) � ������ (1) ������ � �����
   * ���������� ������� ��������. */
  memcpy(buf[2], buf[3], ss*(width+4));
  memcpy(buf[1], buf[2], ss*(width+4));

  /* ������������� �ޣ������ ��� ������� ��������� ������. */
  for (c = c0; c <= cN; c++)
	  init_maketiles_info(&mi[c]);
  
  /* ������� ���� ��������� �����������. ��������� 2 ������ ��� ����
   * �������������� ���������, �������� ��������� � 3-�� ������. */
  for (y = 2; y < height; y++) {
	  /* ����������� ������� ���������� �� ���� ������ �����, ����� ����
	   * � ������ 5-�� ������ (4) ��������� ������ ���������� ������
	   * ������ (0), ������� ����� ���������� ����� �������, ���������� ��
	   * ������������ �����. */
	  tmpbuf = buf[0];
	  buf[0] = buf[1];
	  buf[1] = buf[2];
	  buf[2] = buf[3];
	  buf[3] = buf[4];
	  buf[4] = tmpbuf;

	  rd = freadsmp(buf[4]+2*ss, ss, width, stdin, miniswhite);

	  /* �������� ���������� ����������� ���ޣ��� � ����� � ���������
	   * ������, ���� ���� ��������� ������ ����� ������. */
	  if (rd < width) {
		  fprintf(stderr, "%s: Line %i. Image stream suddenly closed (%i samples has been read)\n", program_name, y, rd);
		  exit(EXIT_FAILURE);
	  }

	  /* ����������� ������� ���ޣ��� ��� ����������� ������� ��������. */
	  edgecpy(buf[4], width, ss);	/* Copy edges of the new line */

	  /* ���������������� ��������� �������� ������� � ���������� ������. */
	  for (c = c0; c <= cN; c++) {
		  maketiles(buf, outbuf, c0, c, ss, width, pos_ascii85_p[c], neg_ascii85_p[c], &mi[c]);
	  }

	  /* �������� ������ ����������� � �������� ���������� ���� ��
	   * ���������� ���������. */
	  write_outbuf(outbuf, ss, width);
  }
  
  /* ���� � ����������� ���� ����� ���� ������, ��� ���������� ����. */
  if (height == 1)
	  memcpy(buf[4], buf[3], ss*(width+4));

  /* ��������� ����� ��� ���� ��������� ����� �����������. */
  for (i = 0; i < (height > 1 ? 2 : 1); i++) {
	  /* ����������� ����� ����������. */
	  tmpbuf = buf[0];
	  buf[0] = buf[1];
	  buf[1] = buf[2];
	  buf[2] = buf[3];
	  buf[3] = buf[4];
	  buf[4] = tmpbuf;

	  /* ����������� ��������� ������ ����, ��� ����������� �������
	   * ��������. */
	  memcpy(buf[4], buf[3], ss*(width+4));
	  
	  /* ���������������� ��������� �������� ������� � ���������� ������. */
	  for (c = c0; c <= cN; c++) {
		  maketiles(buf, outbuf, c0, c, ss, width, pos_ascii85_p[c], neg_ascii85_p[c], &mi[c]);
	  }
	  /* �������� ������ ����������� � �������� ���������� ���� ��
	   * ���������� ���������. */
	  write_outbuf(outbuf, ss, width);
  }
  
  /* ����� ������� ������������ ��� ������� ��������� ������ � ���������������
   * �������� ������. ����� ����������� ������ ���������� PostScript-���������.
   */
  for (c = c0; c <= cN; c++) {
	  /* ����� �������. */
	  ascii85_encode(pos_ascii85_p[c], 0xFF);
	  ascii85_encode(pos_ascii85_p[c], 0xFF);
	  ascii85_encode(pos_ascii85_p[c], 0xFF);
	  ascii85_flush(pos_ascii85_p[c]);
	  ascii85_encode(neg_ascii85_p[c], 0xFF);
	  ascii85_encode(neg_ascii85_p[c], 0xFF);
	  ascii85_encode(neg_ascii85_p[c], 0xFF);
	  ascii85_flush(neg_ascii85_p[c]);

	  /* ����� ����������� ������. */
	  write_footer(pos_outfile[c]);
	  write_footer(neg_outfile[c]);
  }

  /* ������������ ����� �����������, ���� ���� ������� ��� ����� ��� ������. */
  if (histfn != NULL) {
	  histf = fopen(histfn, "w");
	  if (histf != NULL) {
		  fprintf(histf, "#0: %i\n", zerotile);
		  for (i = 0; i < TILE_COUNT; i++)
			  fprintf(histf, "#%i: %i\n", i+1, phist[i]);
		  for (i = 0; i < TILE_COUNT; i++)
			  fprintf(histf, "#-%i: %i\n", i+1, nhist[i]);
		  fclose(histf);
	  } else {
		  /* ����� ��������� �� ������ ��� ���������� ������, ����
		   * ������� ������ ����������� �������. */
		  fprintf(stderr, "Writing histogram to file %s failedi.\n", histfn);
	  }
  }

  /* ��������� �������� �������� ����������. */
  OK = 1;
  
  /* ������������ ������� ��������. */
  (*pop_cleanup())();
  
  /* ����� � ��������� ��������� ����������. */
  exit (0);
  
}
