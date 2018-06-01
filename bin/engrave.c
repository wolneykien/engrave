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


/*
 *
 * ��������� ��� ����������� ������������� ����������� ����������.
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>

#include "system.h"
#include <getopt.h>
#include <tiff.h>
#include <tiffio.h>
#include <math.h>
#include "misc.h"	/* ��������������� ������� */

#ifdef WITH_PDFWRITER
#include "pdfwriter.h"
#endif

#define EXIT_FAILURE 1

/* ��� ���������, ��������� � ���������� ������ */
char *program_name;

/* ���� �������� ������� getopt_long */
enum {DUMMY_KEY=129
     ,BRIEF_KEY
};

/* ����������, ������������ ��������� ���������. */

/* ������� ������� �� ����� ���������� � �������. */
int want_quiet;
/* ������� ������ ������ � ���������� ����������������. */
int want_verbose;
/* ������� ���������� ������  � ������ ����������� ������ ���
 * ��������� �����. */
int exit_on_error;

/* ��������� ����� ��������� �����������. */
char output_name[MAXLINE];	/* ��� �����. */
char suffix[256];	/* ������� �����. */

/* ������� ������ ������ � ������������������ ���������������� �������.
 * ���� --raw */
int is_raw;

/* ��������� ������������������ ����������� */
uint32 width;	/* ������ � �������� */
uint32 height;	/* ������ � �������� */
int miniswhite;	/* ������� �������� ������ �� 0 */
float hres;	/* ���������� �� ����������� � ������ �� ���� */
float vres;	/* ���������� �� ��������� � ������ �� ���� */
int is_cmyk;	/* ������� ������� ���������� � 4 ������� */

/* �������� ��������� � �������������� ��������� ���������� ����������
 * ���������� �����������. */
int want_c;
int want_m;
int want_y;
int want_k;

/* ���������� �������� ��������������� ����������� */
/* ������� ������ ������ �������� ���������� ��������� */
int want_density;
/* ������� ������ ������ ������� */
int want_intensity;

/* ����� ���������� ����� ��������; */
char filter[MAXLINE] = "";

/* ���������� � ������������ ������� �������� */
char filterdir[MAXLINE];
char psdir[MAXLINE]; /* ���������� � ������������� PostScript ������� */

/* ��������� ����� ����������� ����� ����������� */
int want_preview;	/* ������� ������ ����������� �����. */

/* ������� ��������� ������. */
int want_test_run = 0;

/* ������ */
typedef enum { EPS_FMT, TIFF_FMT, PDF_FMT } outformat_t;
outformat_t outformat = EPS_FMT;

/* ������� ������ ���������� ���������� ������ */
static struct option const long_options[] =
{
	{"quiet", no_argument, NULL, 'q'},
	{"silent", no_argument, NULL, 'q'},
	{"verbose", no_argument, NULL, 'v'},
	{"raw", no_argument, NULL, 'r'},
	{"help", no_argument, NULL, 'H'},
	{"version", no_argument, NULL, 'V'},
	{"width", required_argument, NULL, 'w'},
	{"height", required_argument, NULL, 'h'},
	{"hres", required_argument, NULL, 'x'},
	{"vres", required_argument, NULL, 'y'},
	{"cmyk", optional_argument, NULL, 'c'},
	{"density", no_argument, NULL, 'D'},
	{"intensity", no_argument, NULL, 'I'},
	{"filter", required_argument, NULL, 'f'},
	{"filter-path", required_argument, NULL, 'F'},
	{"ps-path", required_argument, NULL, 'P'},
	{"output", optional_argument, NULL, 'o'},
	{"output-suffix", required_argument, NULL, 'O'},
	{"preview", no_argument, NULL, 'p'},
	{"test-run", no_argument, NULL, 'T'},
	{"format", required_argument, NULL, 't'},
	{NULL, 0, NULL, 0}
};

/* ������� ��� ��������� ����������� ���������� ����� � �������� ����.
 * �����������, ���� ����� ���� ���̣�. */
static void
dump_file( FILE *out, const char *fn )
{
	FILE *f = NULL;
	static char str[MAXLINE];

	/* ��������������� ������� ��� �������� ����� � ������
	 * ��������� ��������. */
	void cleanup() {
		if (f != NULL) {
			fclose(f);
			f = NULL;
		}
	}

	/* �������������� ������� �������� ����� � ������� ������� */
	push_cleanup(cleanup);

	/* �������� ���������� ����� */
	f = fopen(fn, "r");
	if (f != NULL) { /* ���� ���� ������ ������ */
		if (want_verbose) /* ����� ��������� � ������������� ������ */
			fprintf(stderr, "Including file %s\n", fn);
		/* ����������� ����� ����� � �������� ����� */
		while (fgets(str, sizeof(str), f) != NULL)
			fputs(str, out);

		/* �������� ����� */
		fclose(f);
	}

	/* ���������, ��� ��������� �������� �� ���������: */
	f = NULL;	/* ������� ��������� �� ����; */
	errno = 0;	/* � ������� ������; */ 
	/* ������� ������� ���������� �������� �� �����. */
	(*pop_cleanup())();
}

/* ����������� ��������������� ���� �������� � �� ��������� ���
 * ��������� ��������.
 *
 * ���������:
 * buf -- ��������� �� ������ ��������� �����������;
 * ss -- ������ ������� � ������;
 * width -- ������ ��������� ����������� � ��������;
 * thumbnail_buf -- ��������� �� ����� ����������;
 * thumbnail_width -- ������ ����������� ����� �����������. */
void get_thumbnail_line(char *buf, size_t ss, size_t width, \
			char *thumbnail_buf, size_t thumbnail_width) {

	double x = 0;
	double step;
	size_t i;
	int sx;

	/* ���������� ���� �������. */
	step = (width - 1)/(double)(thumbnail_width - 1);

	/* ����������� ������ ��������. */
	for (sx = 0; sx < thumbnail_width; sx++) {
		i = ss*rint(x);
		x += step;
		memcpy(thumbnail_buf, buf+i, ss);
		thumbnail_buf += ss;
	}
}

/* ������� ������� ������� � ����������� ������� � ��������� �����
 * ����������. */
void
usage (int status)
{
  printf (_("%s - \
engrave - Adaptive screening of CT images\n\
\n\
Copyright (C) 2018 Yuri V. Kuznetsov, Paul A. Wolneykien.\n\
\n\
This program is free software: you can redistribute it and/or modify\n\
it under the terms of the GNU Affero General Public License as\n\
published by the Free Software Foundation, either version 3 of the\n\
License, or (at your option) any later version.\n\
See http://www.gnu.org/licenses/ for details.\n\n"), program_name);
  printf (_("Usage: %s [OPTIONS] -f FILTER [option] [-f FILTER [option]...] [FILE]\n"), \
	  program_name);
  printf (_("\n\
Options:\n\
  -r, --raw			process RAW image stream\n\
  -w WIDTH, --width=WIDTH	RAW image width\n\
  -h HEIGHT, --height=HEIGHT	and height\n\
  -x HRES, --hres=HRES		RAW image horizontal\n\
  -y VRES, --vres=VRES		and vertical resolution\n\
  -c [CMYK], --cmyk[=CMYK]	image is CMYK; optionally selects\n\
                                individual colorants;\n\
  -D, --density			input and output data is DENSITY\n\
                                values\n\
  -I, --intensity		intput and output data is INTENSITY\n\
                                values\n\
  -F FPATH --filter-path=FPATH	path to filters, default is '%s'\n\
  -P PSPATH --ps-path=PSPATH	path to PostScript library files,\n\
                                default is '%s'\n\
  -o [NAME], --output[=NAME]	write output to file [NAME] (default\n\
                                <NAME>.<SUF>)\n\
  -O SUF, --output-suffix=SUF	write output file to <NAME>.SUF \n\
                                (default 'eps')\n\
  -p, --preview			add preview image to the EPS\n\
  -T, --test-run        keep temporary files\n\
  -t FMT, --format=FMT  output format (eps, tiff)\n\
  -H, --help			display this help and exit\n\
  -v, --verbose			verbose message output\n\
  -V, --version			output version information and exit\n\
\n\
Filter chain:\n\
  -f FILTER [options], \n\
  --filter=FILTER [options]     process image through FILTER [filter\n\
                                options].\n\
"), filterdir, psdir);
  exit (status);
}

/* ��������� ���������� ���������� ������ ������� ��
 * ��������� ��������. */
int
decode_filter_switches(int argc, char **argv)
{

  /* ���������� optind ��������� �� ������ ��������������
   * ��������. */
  int i = optind;
  char *arg;

  /* ������ ���������� ���������� ������. */

  /* ����������� ����� ������� � ������ ��������� ������. */
  strcpy(filter, argv[i - 1]);

  /* ���������������� ��������� ����������. */
  while (i < argc) {
	arg = argv[i];
	/* ��������, �������� �� �������� �������� */
	if (arg[0] != '-') {
		/* ���������� ������� ������� ����������. */
		strcat(filter, "\n");
		break;
	} else if (strcmp(arg, "-f") == 0) {
		/* ������ �������� ������ ��������� �������� ����������. */
		strcat(filter, "\n");
		/* �������� �� ������� ���������� ���������. */
		if (i == argc) {
			/* ����� ��������� �� ������ */
			fprintf(stderr, "No filter name specified\n");
			usage(EXIT_FAILURE); /* ������ ������� � ����� */
		} else {
			/* ������� � ���������� ���������. */
			i++;
		}
		/* ��� ���������� ������� �����������
		 * � ������ ����� ������. */
		strcat(filter, argv[i]);
	} else {
		strcat(filter, " "); /* ���������� ������ �������� */
		strcat(filter, arg); /* ���������� ��������� */
	}

	/* ������� � ���������� ���������. */
	i++;
  }

  return i;
}

/* ��������� �������� ���������� � ������������ � �����������
 * ���������� ������. ������� ���������� ����� ������� ���������,
 * �� ����������� ������. */
int
decode_switches (int argc, char **argv)
{
  int c;
  int option_index;
  char *endptr;

  /* �������� �������� ���������� �� ���������. */
  is_cmyk = 0;
  want_c = 0;
  want_m = 0;
  want_y = 0;
  want_k = 0;
  want_density = 0;
  want_intensity = 0;
  width = 0;
  height = 0;
  vres = 0;
  hres = 0;
  miniswhite = 0;
  want_verbose = 0;
  want_quiet = 0;
  is_raw = 0;
  filter[0] = '\0';
  suffix[0] = '\0';
  output_name[0] = '\0';
  want_preview = 0;
  want_test_run = 0;
  outformat = EPS_FMT;

  /* ������� ���������� ���������� ������ � ������� ������� getopt_long. */
  while ((c = getopt_long (argc, argv, 
			   "r"  /* raw */
			   "f:"  /* filter */
			   "F:"  /* filter path */
			   "P:"	/* pslib path */
			   "q"  /* quiet */
			   "v"  /* verbose */
			   "w:"  /* width */
			   "h:"  /* height */
			   "x:"  /* hres */
			   "y:"  /* vres */
			   "c::"  /* cmyk */
			   "D"	/* density */
			   "I" /* intensity */
			   "H"	/* help */
			   "V"	/* version */
			   "o::" /* output to file */
			   "O:" /* output suffix */
			   "p"  /* add preview */
			   "T"  /* test run */
			   "t:", /* output format */
			   long_options, &option_index)) != EOF)
    {
      switch (c) /* ������ ���������� ���� ���������. */
	{

	/* ����� ������ ������ � ����������������� ������������. */
	case 'r':
		is_raw = 1;
		break;

	/* ����� ���������� � ������������ ������� ��������. */
	case 'F':
		snprintf(filterdir, sizeof(filterdir), optarg);
		snprintf(psdir, sizeof(psdir), optarg);
		break;

	/* ����� ���������� � ������������� PostScript �������. */
	case 'P':
		snprintf(psdir, sizeof(psdir), optarg);
		break;

	/* ����� ������ � ����. */
	case 'o':
		if (optarg != NULL)
			strcpy(output_name, optarg);
		break;

	/* ����� �������� ����� ����������. */
	case 'O':
		strcpy(suffix, optarg);
		break;
	
	/* ����� ���������� ����������� ����� � ����. */
	case 'p':
		want_preview = 1;
		break;

	/* ����� ������ ���������� ��������������� ���������. */
	case 'q':
		want_quiet = 1;
		break;

	/* ����� ������ ���������� ��������������� ���������. */
	case 'v':
		want_verbose = 1;
		break;
		
	/* ������� ������ �����������. */
	case 'w':
		/* ������� �������������� �������� � �����. */
		width = strtoul(optarg, &endptr, 0);
		/* ����� � ������� ��������� �� ������ � ������ �������. */
		if (errno == ERANGE || *endptr != '\0') {
			fprintf(stderr, "%s", "Width value is invalid.\n");
			exit(EXIT_FAILURE);
		}
		break;
		
	/* ������� ������ �����������. */
	case 'h':
		/* ������� �������������� �������� � �����. */
		height = strtoul(optarg, &endptr, 0);
		/* ����� � ������� ��������� �� ������ � ������ �������. */
		if (errno == ERANGE || *endptr != '\0') {
			fprintf(stderr, "%s", "Height value is invalid.\n");
			exit(EXIT_FAILURE);
		}
		break;

	/* ������� ��������������� ���������� �����������. */
	case 'x':
		/* ������� �������������� �������� � �����. */
		hres = strtod(optarg, &endptr);
		/* ����� � ������� ��������� �� ������ � ������ �������. */
		if (errno == ERANGE || *endptr != '\0') {
			fprintf(stderr, "%s", "HRES value is invalid.\n");
			exit(EXIT_FAILURE);
		}
		break;

	/* ������� ������������� ���������� �����������. */
	case 'y':
		/* ������� �������������� �������� � �����. */
		vres = strtod(optarg, &endptr);
		/* ����� � ������� ��������� �� ������ � ������ �������. */
		if (errno == ERANGE || *endptr != '\0') {
			fprintf(stderr, "%s", "VRES value is invalid.\n");
			exit(EXIT_FAILURE);
		}
		break;

	/* ����� ������ ����ң����������� �����������. */
	case 'c':
		is_cmyk = 1;
		/* �������� ������ �� 0, ���� �� ������� ���������. */
		if (!want_intensity) {
			miniswhite = 1;
		}
		if (optarg != NULL) {
		  if (strchr(optarg, 'C') != NULL) {
		    want_c = 1;
		  }
		  if (strchr(optarg, 'M') != NULL) {
		    want_m = 1;
		  }
		  if (strchr(optarg, 'Y') != NULL) {
		    want_y = 1;
		  }
		  if (strchr(optarg, 'K') != NULL) {
		    want_k = 1;
		  }
		}
		break;

	/* ����� ������ ������ �������� ���������� ���������. */
	case 'D':
		if (!want_intensity) {
			miniswhite = 1; /* �������� ������ �� 0. */
			want_density = 1;
		} else {
		/* ��������� �� ������, ���� ��������� �������� ����������. */
			fprintf(stderr, "INTNSITY and DENSITY are mutually exclusive options\n");
			exit(EXIT_FAILURE); /* ����� */
		}
		break;

	/* ����� ������ ������ �������� �������. */
	case 'I':
		if (!want_density) {
			miniswhite = 0; /* �������� ޣ����� �� 0. */
			want_intensity = 1;
		} else {
		/* ��������� �� ������, ���� ��������� �������� ����������. */
			fprintf(stderr, "INTNSITY and DENSITY are mutually exclusive options\n");
			exit(EXIT_FAILURE); /* ����� */
		}
		break;

	/* ������ ������ ������ � �����. */
	case 'V':
	  printf ("engrave %s\n", VERSION);
	  exit (0);

	/* ������ ������� ������� � �����. */
	case 'H':
	  usage (0);

	/* ������ ���������� ����� ��������.
	 * �� ���� ������ ���������� �����������.
	 * ������� ������� ���������� ������� ����������
	 * ����� ������� ����������� ���������. */
	case 'f':
		return decode_filter_switches(argc, argv);

	/*
	 * �������� ������ (�� ������� ��������� �����).
	 */
	case 'T':
	  want_test_run = 1;
	  break;

	case 't':
	  if ( 0 == strcmp( optarg, "eps" ) ||
	  	   0 == strcmp( optarg, "EPS" ) )
	  	{
	  		outformat = EPS_FMT;
			break;
	  	}
	  if ( 0 == strcmp( optarg, "tiff" ) ||
	  	   0 == strcmp( optarg, "TIFF" ) )
	  	{
	  		outformat = TIFF_FMT;
			break;
	  	}
	  if ( 0 == strcmp( optarg, "pdf" ) ||
	  	   0 == strcmp( optarg, "PDF" ) )
	  	{
	  		outformat = PDF_FMT;
			break;
	  	}

	/* ���� �������� �� ��� ���������, �� ���������� ���������,
	 * ��������� ������� ������� � ������������ �����
	 * � ����� ������. */
	default:
	  usage (EXIT_FAILURE);
	}
    }

  /* ������� �������� ������� ���������� optind, �����������
   * �� ����� ���������� ��������������� ���������. */
  return optind;
}

/* ������� �������. */
int
main (int argc, char **argv)
{

  int opt_r;	/* ��������� ������ decode_options(). */
  int retc;	/* ��� �������� �� ������� ���������. */

  /* ��������� ����� ���������. */
  program_name = argv[0];

  char pdirbuf[1024];
  snprintf(pdirbuf, sizeof(pdirbuf), "%s", argv[0]);
  dirname(pdirbuf);

  if (strcmp(pdirbuf, argv[0]) == 0) {
    /* ������ �� ������� ���������� (��� Win)? */
    pdirbuf[0] = '\0';
  }

  /* ��� ������� ���������� �� ��������� ������������ �������� ��������.
     ���� ���� �������������, �� ����������� ���� � ���������. */

#ifdef FILTERS
#  ifndef __MINGW32__
  if (strncmp(FILTERS, "/", 1) == 0)
    snprintf(filterdir, sizeof(filterdir), "%s", FILTERS);
  else
    snprintf(filterdir, sizeof(filterdir), "%s/%s", pdirbuf, FILTERS);
#  else
  if (strlen(pdirbuf) > 0)
    snprintf(filterdir, sizeof(filterdir), "%s\\%s", pdirbuf, FILTERS);
  else
    snprintf(filterdir, sizeof(filterdir), "%s", FILTERS);
#  endif
#else
  snprintf(filterdir, sizeof(filterdir), "%s", pdirbuf);
#endif

#ifdef PSLIB
#  ifndef __MINGW32__
  if (strncmp(PSLIB, "/", 1) == 0)
    snprintf(psdir, sizeof(psdir), "%s", PSLIB);
  else
    snprintf(psdir, sizeof(psdir), "%s/%s", pdirbuf, PSLIB);
#  else
  if (strlen(pdirbuf) > 0)
    snprintf(filterdir, sizeof(filterdir), "%s\\%s", pdirbuf, PSLIB);
  else
    snprintf(filterdir, sizeof(filterdir), "%s", PSLIB);
#  endif
#else
  snprintf(psdir, sizeof(psdir), "%s", pdirbuf);
#endif
  
  /* ������ ���������� ���������� ������. */
  opt_r = decode_switches (argc, argv);

  /* ���� �� ������� �� ������ ��������� �����,
   * �� ������� ��������� ���������� � ������ ����������,
   * ����������, ��� ������� ������������ �����������������
   * ������, ���������� �� ����������� ����. */
  if (opt_r == argc)
  	return process(NULL);

  /* ���������������� ��������� ������, ��������� � ���������� ������. */
  while (opt_r < argc) {
  	retc = process(argv[opt_r]);
	if (retc && exit_on_error) {
		return retc;
	}

	/* ����� ����� ��������� �����. */
	output_name[0] = '\0';

  	opt_r++;
  }

  return 0;
}

/* ��������������� �������.
 * ��������� ������ ����� ����� �� �������,
 * ��ԣ� ������ ��������. */
void
set_suffix(char *destname, const char *filename, const char *suffix)
{

  char *dot;

  dot = strrchr(filename, '.'); 
  if (dot != NULL)
  	dot[0] = '\0';
  snprintf(destname, MAXLINE, "%s.%s", filename, suffix);
  if (dot != NULL)
  	dot[0] = '.';
  
}

/* ��������������� �������.
 * ����������� �������� ����� �������. */
void
filter_basename(char *filter_name, const char *a_filter)
{
	char *filter_tail;

	filter_tail = strchr(a_filter, ' ');
	if (filter_tail != NULL) {
		*filter_tail = '\0';
	}
	strcpy(filter_name, a_filter);
	if (filter_tail != NULL) {
		*filter_tail = ' ';
	}
}

struct output_ctx {
	FILE *output_file;
	void *pdfctx;
};

static void
close_output( struct output_ctx* ctx )
{
	if ( ctx ) {
		if ( ctx->output_file ) {
			if ( ctx->output_file != stdout ) {
				if ( fclose(ctx->output_file) && want_verbose ) {
					fprintf(stderr, "Failed to close the output file\n");
				}
			}
			ctx->output_file = NULL;
		}
		if ( ctx->pdfctx ) {
			if ( pdf_close( ctx->pdfctx ) && want_verbose ) {
				fprintf(stderr, "Failed to close the output PDF file\n");
			}
			ctx->pdfctx = NULL;
		}
		free( ctx );
	}
}

/* ������� �������� ������������ ����� ���������� �
 * ���������� ������ � ������. */
static int
prepare_output( const char *file_name, char *output_name,
				struct output_ctx **outctx )
{
  if ( outformat == TIFF_FMT ) return 0;
	
  /* ���������� ������.
   * ����������� �ͣ� � ������������ � ���������� ����������. */

  /* ����������� ����� ����������.
   * ���� ��� ��� ������� �� �������, �� ������������
   * �������� �� ���������. */
	
   if (suffix[0] == '\0') {
	   switch ( outformat ) {
	   case PDF_FMT:
		   strcpy(suffix, "pdf");
		   break;
	   default:
		   strcpy(suffix, "eps");
	   }
   }
   
   if (strlen(output_name) == 0) {
	   if (file_name != NULL) {
		   set_suffix(output_name, file_name, suffix);
	   } else {
		   set_suffix(output_name, "output", suffix);
	   }
   }

   *outctx = malloc( sizeof(*outctx) );
   if ( !*outctx ) return EXIT_FAILURE;
   (*outctx)->output_file = NULL;
   (*outctx)->pdfctx = NULL;
   
   switch ( outformat ) {
   case PDF_FMT:
	   (*outctx)->pdfctx =
		   pdf_open_file( output_name,
						  ((double) width / (double) hres) * 72,
						  ((double) height / (double) vres) * 72 );
	   if ( !(*outctx)->pdfctx )
		   return EXIT_FAILURE;
	   break;
   default:
	   if (strcmp(output_name, "-") == 0) {
		   (*outctx)->output_file = stdout;
	   } else {
		   if (want_preview) {
			   strcat(output_name, "~");
		   }
		   (*outctx)->output_file = fopen(output_name, "w");
		   if ( !(*outctx)->output_file ) {
			   return EXIT_FAILURE;
		   }
	   }
   }

   if (want_verbose) {
	   fprintf(stderr, "\nProcessing file %s\nOutput file is %s\n",	\
			   (file_name != NULL ? file_name : "-"),				\
			   (output_name != NULL ? output_name : "-"));
   }

   /* ������� ��������� ���������� �������. */
   return 0;
}

/* ��������������� ������� ��� �������� ����� ����������������
 * ����������� � ������� TIFF. */
TIFF *
open_tif_file(char *file_name, uint16 *phm, tsample_t *spp, uint16 *r_unit, uint16 *tiff_planar)
{

  TIFF *tif;

  if (!is_raw) {
  	/* �������� ����� ����������� � ������� TIFF. */
	  tif = TIFFOpen(file_name, "r");
	  /* ��������� �� ������ ���� �� ������� ������� ����. */
	  if (tif == NULL) {
		  fprintf(stderr, "Error open TIFF file %s\n", file_name);
		  exit(EXIT_FAILURE); /* ����� */
	  }

	  /* ������ ����� ����������� � ������ �� � ����������. */
	  TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
	  TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
	  TIFFGetField(tif, TIFFTAG_XRESOLUTION, &hres);
	  TIFFGetField(tif, TIFFTAG_YRESOLUTION, &vres);
	  TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, phm);
	  TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, spp);
	  TIFFGetField(tif, TIFFTAG_PLANARCONFIG, tiff_planar);

	  /* ��������� ������ ������ � ������������
	   * �� ���������� �����. */
	  switch (*spp) {
		  case 1: /* � ������ 1 ����� �� ������. */
			  if (*phm == PHOTOMETRIC_MINISWHITE) {
				  miniswhite = 1;
			/* ����������� �� ����� ���� ��������������. */
				  is_cmyk = 0;
			  }
			  break;
		  /* 4 ����� �� ������� �������� 4 ��������� �����������. */
		  case 4: 
			  if (*phm == PHOTOMETRIC_SEPARATED) {
				  miniswhite = 1;
				  is_cmyk = 1;
			  } else {
			  /* �� ��� ����� ����� ���� � ����������� ��������
			   * ������������. */
				  fprintf(stderr, "Four channel image is not separated...Exit\n");
				  exit(EXIT_FAILURE);
			  }
			  break;
		  default:
		  	/* ����� ������ ���������� ���ޣ��� �� �������
			 * ��������� ���������. */
			  fprintf(stderr, "Illegal samples per pixel (number of channels): %i\n", *spp);
			  exit(EXIT_FAILURE);
	  }

	  /* ����������� ���������� �����������.
	   * ����������� ������� ��������� ����������. */
	  TIFFGetField(tif, TIFFTAG_RESOLUTIONUNIT, r_unit);
	  /* ���������� ���������� � ������ �� ����, � ��� ������
	   * ���� ��� ������� � ������ �� ���������. */
	  if (*r_unit == RESUNIT_CENTIMETER) {
		  hres /= 2.54;
		  vres /= 2.54;
	  }
  }

  return tif;

}

void
copy_file(FILE *dst, FILE *src)
{

	/* ������������� ����� � ����������� �ޣ����� ��� ��������
	 * ����/������. */
	char buf[4096];
	int rd;
	int wt;
	long total = 0;

	/* ����������� ����� ����� ������������� �����. */
	rd = fread(buf, 1, sizeof(buf), src);
	while (rd > 0) {
		wt = fwrite(buf, 1, rd, dst);
		total += wt;
		/* ���� ����� ��� ������� �� �������, �� ������������ �����
	 	 * � ��������� ������. */
		if (wt != rd) {
			fprintf(stderr, "Unable to write to file\n");
			exit(EXIT_FAILURE);
		}
		rd = fread(buf, 1, sizeof(buf), src);
	}
	if (want_verbose) {
	  fprintf(stderr, "%li bytes written\n", total);
	}
}

void
add_preview (char *psname, char *tiffname)
{

  	/* ��������� ��� �������� ���������� � ������: */
	struct stat psstat;	/* � EPS �����; */
	struct stat tiffstat;	/* � TIFF �����. */
	
	/* ��������� �������� EPS-����� (EPSF). */
	struct header_s {
		unsigned char info[4];
		unsigned char psstart[4];
		unsigned char pslength[4];
		unsigned char mfstart[4];
		unsigned char mflength[4];
		unsigned char tiffstart[4];
		unsigned char tifflength[4];
		unsigned char check[2];
	} header;

	/* ��������� ��� ������� � �������� ������. */
	FILE *eps = NULL;
	FILE *tif = NULL;

	/* ��� ��������������� ����� */
        char resname[MAXLINE] = "";

	/* ��������� ��� ������� � ��������������� �����. */
	FILE *res = NULL;

	/* ����������� �ޣ���� ��� �������� ������ � ����. */
	int wt;

	/* ������� ��������� ������� ������� ��������. */
	void cleanup() {
	  if (eps != NULL) {
	    fclose(eps);
	    if (!unlink(psname) && want_verbose) {
	      fprintf(stderr, "Delete temporary file %s\n", psname);
	    }
	  }
	  if (tif != NULL) {
	    fclose(tif);
	    if (!want_test_run) {
	      if (!unlink(tiffname) && want_verbose) {
		fprintf(stderr, "Delete temporary file %s\n", tiffname);
	      }
	    } else {
	      fprintf(stderr, "Test run: temporary file %s not deleted.\n", tiffname);
	    }
	  }
	  if (res != NULL) {
	    fclose(res);
	  }
	}

	/* ������������� ��������� ���������� ������������ ��������. */
	push_cleanup(cleanup);

	/* ��������� ���������� � EPS-�����. */
	if (stat(psname, &psstat)) {
		fprintf(stderr, "Can't get attributes of file %s\n", psname);
		exit(EXIT_FAILURE);	/* ������� � ������ ������. */
	}
	/* ��������� ���������� � TIFF-�����. */
	if (stat(tiffname, &tiffstat)) {
		fprintf(stderr, "Can't get attributes of file %s\n", tiffname);
		exit(EXIT_FAILURE);	/* ����� � ������ ������. */
	}

	/* ���������� ��������� ��������� ����� � ������������ ��
	 * ������������� EPSF. */
	header.info[0] = 0xC5;
	header.info[1] = 0xD0;
	header.info[2] = 0xD3;
	header.info[3] = 0xC6;
	
	header.psstart[0] = sizeof(header);
	header.psstart[1] = 0;
	header.psstart[2] = 0;
	header.psstart[3] = 0;

	/* ������������� ���������� ���������� � ������ ��� ����������
	 * ���������. */
	header.tifflength[0] = (unsigned char)(tiffstat.st_size & 0xFF);
	header.tifflength[1] = (unsigned char)((tiffstat.st_size >> 8) & 0xFF);
	header.tifflength[2] = (unsigned char)((tiffstat.st_size >> 16) & 0xFF);
	header.tifflength[3] = (unsigned char)((tiffstat.st_size >> 24) & 0xFF);
	
	header.tiffstart[0] = (unsigned char)((psstat.st_size + sizeof(header)) & 0xFF);
	header.tiffstart[1] = (unsigned char)(((psstat.st_size + sizeof(header)) >> 8) & 0xFF);
	header.tiffstart[2] = (unsigned char)(((psstat.st_size + sizeof(header)) >> 16) & 0xFF);
	header.tiffstart[3] = (unsigned char)(((psstat.st_size + sizeof(header)) >> 24) & 0xFF);
	
	header.pslength[0] = (unsigned char)(psstat.st_size & 0xFF);
	header.pslength[1] = (unsigned char)((psstat.st_size >> 8) & 0xFF);
	header.pslength[2] = (unsigned char)((psstat.st_size >> 16) & 0xFF);
	header.pslength[3] = (unsigned char)((psstat.st_size >> 24) & 0xFF);

	header.mfstart[0] = 0;
	header.mfstart[1] = 0;
	header.mfstart[2] = 0;
	header.mfstart[3] = 0;

	header.mflength[0] = 0;
	header.mflength[1] = 0;
	header.mflength[2] = 0;
	header.mflength[3] = 0;

	/* ��������� �������� ���������� ����������� ����� ��������. */
	header.check[0] = 0xFF;
	header.check[1] = 0xFF;

	/* �������� ����� ����������� �����. */
	tif = fopen(tiffname, "r");
	if (tif == NULL) {
		fprintf(stderr, "Can't open file %s\n", tiffname);
		exit(EXIT_FAILURE);	/* ����� � ������ ������. */
	}

	/* �������� PostScript-�����. */
	eps = fopen(psname, "r");
	if (eps == NULL) {
		fprintf(stderr, "Can't open file %s\n", psname);
		exit(EXIT_FAILURE);	/* ����� � ������ ������. */
	}
	
	/* ���������� ����� ����� �� ���� ������, ��������� ����� � ����
	 * ��� �������� ������ '~', ����� ������ ����������� ������������
	 * ��������� � ���������� ������ ��� � �������� ����� ���������������
	 * �����. */
	strncpy(resname, psname, strlen(psname) - 1);

	/* �������� ��������������� ����� �� ������ */
	res = fopen(resname, "w+");
	if (res == NULL) {
		fprintf(stderr, "Can't open file %s\n", resname);
		exit(EXIT_FAILURE);	/* ����� � ������ ������. */
	}

	/* ������ ��������� � �������������� ����. */
	if (want_verbose) {
		fprintf(stderr, "Writing EPSF header... \n");
	}
	wt = fwrite(&header, 1, sizeof(header), res);

	/* ���� �������� ��� ������� �� �������, �� ������������ �����
	 * � ��������� ������. */
	if (wt < sizeof(header)) {
		fprintf(stderr, "Unable to write header to %s\n", resname);
		exit(EXIT_FAILURE);
	}

	/* ������ Post-Script-�����. */
	if (want_verbose) {
		fprintf(stderr, "Writing PS... \n");
	}
	copy_file(res, eps);

	/* ������ ����������� ����� �����������. */
	if (want_verbose) {
		fprintf(stderr, "Writing preview... \n");
	}
	copy_file(res, tif);

	if (want_verbose) {
		fprintf(stderr, "Unlink files... \n");
	}

	/* ���������� ��������� ������������ ������� ��������. */
	/* �������� ������������� ������. */
  	(*pop_cleanup())();

	if (want_verbose) {
		fprintf(stderr, "EPSF done.\n");
	}

}

/* ����������� ���������� ������ ��� ������ ��������. */
void
parse_filters(char *f_cmd, int *filter_count, pid_t pid)
{

  /* ���������� ��� ������ � ����������� �������� ��������. */
  char *next_filter;
  char *a_filter;
  char i_arg[32];
  char f_path[MAXLINE];
  char f_args[MAXLINE];

  char *outformat_str = NULL;

  /* �������������� ���������� ������� � ���������
     �������������. */
  switch ( outformat ) {
  case EPS_FMT:
	  outformat_str = "eps";
	  break;
  case TIFF_FMT:
  case PDF_FMT:
	  outformat_str = "tiff";
	  break;
  default:
	  fprintf( stderr, "BUG: Unexpected output format: %d\n",
			   outformat );
	  exit(EXIT_FAILURE);
  }

  /* ������������ ���������� ������ ��� ������ ��������. */
  /* ������� ����� ��� ���� �������� ����������, ������������
   * ��������� ��������������� ����������� ���������������
   * ������������. */
  snprintf(f_args, sizeof(f_args), " -p %u -w %u -h %u -x %.2f -y %.2f -t %s", pid, width, height, hres, vres, outformat_str);
  
  /* ���������� �������� 4 ���������� �����������. */
  if (is_cmyk)
	strcat(f_args, " -c");
  /* ���������� �������� ����������� �����������. */
  if (want_density || !want_intensity && miniswhite)
	  strcat(f_args, " -D");
  else
	  strcat(f_args, " -I");

  /* ���������� �������� ���������� ��������������� ��� ��������, ���� ��
   * ����� ��� �������� ���������. */
  if (want_verbose) {
	strcat(f_args, " -v");
  }

  /* ��������� ��������� ���������. */
  next_filter = filter;

  /* ������� ���������� ������ �� ������� �����. */
  f_cmd[0] = '\0';

  *filter_count = 0;
  
  /* ��������� ����� ������ �� ������� ��������. */
  char *saveptr = NULL;
  a_filter = strtok_r(next_filter, "\n", &saveptr);
  while (a_filter != NULL) {
    /* ���� ���������� ������ ��� �������� ��������� ��������,
     * � ������ ���������� ������ ����������� ������ ������������. */
    if (strlen(f_cmd) > 0)
      strcat(f_cmd, " | ");
    
    /* ������ ���������� ������. */
    strcpy(f_path, filterdir);
    pathcat(f_path, a_filter);
    strcat(f_cmd, f_path);
    strcat(f_cmd, f_args);
    
    /* ���������� ����� � ������� �������. */
    snprintf(i_arg, sizeof(i_arg), " -i %u", (*filter_count)++);
    strcat(f_cmd, i_arg);

    /* ���������� �������� �� ��������� ��������. */
    a_filter = strtok_r(NULL, "\n", &saveptr);
  }

  /* TODO: ������ ������ � "�������" ������� ��-�������. */
#ifndef __MINGW32__
  strcat(f_cmd, " > /dev/null");
#endif
}

/* ������� ��� ������ ��������� PostScript-���������. */
void
ps_header(const char *file_name, FILE *output_file)
{

  /* ���������� ��� �������� �������� �������. */
  time_t cr_time;
  char cr_time_str[256];

  /* ���������� ��� ������ � ����������� �������� ��������. */
  char *next_filter;
  char *a_filter;
  char filter_name[256];

  /* ��������� ��� ����������� ���� � ������������ PostScript-������. */
  char ps_path[MAXLINE];

  /* ����������� �������� �������. */
  cr_time = time(NULL);
  /* �������������� ����������� �������� �������. */
  strftime(cr_time_str, sizeof(cr_time_str), "%Y-%m-%d %H:%M:%S", localtime(&cr_time));

  /* ����� ��������� PostScript ��������� ��� ����������
   * ��������� �����������. */
  fprintf(output_file,
  	 "%%!PS-Adobe-3.0 EPSF-3.0\n"
	 "%%%%Creator: "PACKAGE" "VERSION". Adaptive Screening Technology.\n"
	 "%%%%Title: %s\n"
	 "%%%%CreationDate: %s"
	 "%%%%DocumentData: Clean7Bit\n"
	 "%%%%LanguageLevel: 2\n"
	 "%%%%Pages: 1\n"
	 "%%%%BoundingBox: 0 0 %.0f %.0f\n"
	 "%%%%DocumentProcessColors: %s\n"
	 "%%%%EndComments\n"
	 "%%%%BeginProlog\n"
	 "%% Use own dictionary to avoid conflicts\n"
	 "40 dict begin\n",
	 file_name,		/* ��� �����. */
	 cr_time_str,		/* ������� �����. */
	 (float) width/hres*72,		/* ������ */
	 (float) height/vres*72,	/* � ������ � �������. */
	 /* �������� ���������. */
	 is_cmyk ? "Cyan Magenta Yellow Black" : "Black");

  /* ���������� ������������ PostScript ������ � �������� ���������. */
  
  /* ��������� ��������� ���������. */
  next_filter = filter;
  
  /* ��������� ����� ������ �� ������� ��������. */
  char *saveptr = NULL;
  a_filter = strtok_r(next_filter, "\n", &saveptr);
  while (a_filter != NULL) {
    filter_basename(filter_name, a_filter);
    strcpy(ps_path, psdir);
    pathcat(ps_path, filter_name);

    /* ����������� ������������� PostScript ����� � �������� �����. */
    dump_file(output_file, strcat(ps_path, ".ps"));

    /* ���������� �������� �� ��������� ��������. */
    a_filter = strtok_r(NULL, "\n", &saveptr);
  }

  /* ���������� ��������� PostScript ���������. */
  fprintf(output_file,	"%%%%EndProlog\n"
	 		"%%%%Page: 1 1\n"
	 		"gsave\t%% Save grafics state\n"
	 		"true setoverprint\n");
}

/* ���������� ���������� ����� ����������� �����. */
void
prepare_preview(char *thumbnail_name, pid_t pid, uint16 phm, int ss, uint16 tiff_planar, TIFF **thumbnail, char **thumbnail_buf, uint32 *thumbnail_width, uint32 *thumbnail_height)
{

  /* �������� ����� ����������� ����� ����������� �� ������.
   * ��������� ������. */
  if (want_preview) {
  	  snprintf(thumbnail_name, MAXLINE, "%i.prv.tif", pid);
	  *thumbnail = TIFFOpen(thumbnail_name, "w");
	  if (thumbnail == NULL) {
		  fprintf(stderr, "Can't write thumbnail file %s\n", "");
		  exit(EXIT_FAILURE);
	  }
  }

  /* ���������� �������� ����������� ����� ����������� ������ �� ����,
   * ��� ţ ���������� ������ ��������� 72 ������ �� ����. */
  if (*thumbnail != NULL) {
	  if (hres > 72)
		  *thumbnail_width = rint(width*(72/hres));
	  else /* ���������� �� ������ �� ���������. */
		  *thumbnail_width = width;
	  if (vres > 72)
		  *thumbnail_height = rint(height*(72/vres));
	  else /* ���������� �� ������ �� ���������. */
		  *thumbnail_height = height;
	  
  	  /* ��������� ������ ��� �������� ����� ����������� �����. */
	  *thumbnail_buf = _TIFFmalloc(ss * *thumbnail_width);
	  if (*thumbnail_buf == NULL) {
		  fprintf(stderr, "Thumbnail scanline buffer allocation failed\n");
		  exit(EXIT_FAILURE);
	  }

	  /* ������� ����� ����������� ����������� �����. */
	  TIFFSetField(*thumbnail, TIFFTAG_PLANARCONFIG, tiff_planar);
	  TIFFSetField(*thumbnail, TIFFTAG_SAMPLESPERPIXEL, ss);
	  TIFFSetField(*thumbnail, TIFFTAG_BITSPERSAMPLE, 8);
	  TIFFSetField(*thumbnail, TIFFTAG_IMAGEWIDTH, *thumbnail_width);
	  TIFFSetField(*thumbnail, TIFFTAG_IMAGELENGTH, *thumbnail_height);
	  TIFFSetField(*thumbnail, TIFFTAG_XRESOLUTION, hres < 72 ? hres : 72);
	  TIFFSetField(*thumbnail, TIFFTAG_YRESOLUTION, vres < 72 ? vres : 72);
	  TIFFSetField(*thumbnail, TIFFTAG_PHOTOMETRIC, phm);
	  TIFFSetField(*thumbnail, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);

  }

}

static void
delete_temporary_filter_file( const char *fsuf, pid_t pid,
							  int fidx, int color_idx )
{
	const char *tmp_fn = NULL;
	
	tmp_fn = get_tmp_file_name( fsuf, pid, fidx, color_idx );
	if ( !tmp_fn ) {
		fprintf(stderr, "Unable to get the name of a filter "	\
				"temporary file\n");
	} else {
		if ( !want_test_run ) {
			if ( unlink(tmp_fn) ) {
				fprintf(stderr, "Unable to delete temporary file\n");
			} else if ( want_verbose ) {
				fprintf(stderr, "Delete temporary file %s\n", tmp_fn);
			}
		} else {
			fprintf(stderr,
					"Test run: temporary file %s not deleted.\n",
					tmp_fn);
		}
		free( tmp_fn ); tmp_fn = NULL;
	}
}

static void
use_temporary_filter_file( struct output_ctx *outctx,
						   const char *fsuf, pid_t pid,
						   int fidx, int color_idx )
{
	const char *tmp_fn = NULL;
	pdfcolor_t pdf_color;
	
	tmp_fn = get_tmp_file_name( fsuf, pid, fidx, color_idx );
	if ( !tmp_fn ) {
		fprintf(stderr, "Unable to get the name of a filter "	\
				"temporary file\n");
		return;
	}

	struct stat statbuf;
	if ( stat( tmp_fn, &statbuf ) != 0 ) {
		free( tmp_fn );
		return;
	}

	switch ( outformat ) {
	case PDF_FMT:
		switch ( color_idx ) {
		case 0:
			pdf_color = PDFCOLOR_CYAN;
			break;
		case 1:
			pdf_color = PDFCOLOR_MAGENTA;
			break;
		case 2:
			pdf_color = PDFCOLOR_YELLOW;
			break;
		case 3:
			pdf_color = PDFCOLOR_BLACK;
			break;
		}
		if ( outctx->pdfctx ) {
			if ( strcmp( fsuf, "ct" ) == 0 ) {
				pdf_add_tonemap( outctx->pdfctx, tmp_fn, pdf_color );
			} else if ( strcmp( fsuf, "m" ) == 0 ) {
				pdf_add_bitmap( outctx->pdfctx, tmp_fn, PDFCOLOR_WHITE );
			} else if ( strcmp( fsuf, "s" ) == 0 ) {
				pdf_add_bitmap( outctx->pdfctx, tmp_fn, pdf_color );
			} else {
				fprintf( stderr,
						 "Error: Unknown filter output class: %s\n",
						 fsuf );
			}
		}
		break;
	default:
		dump_file( outctx->output_file, tmp_fn );
	}
	
	free( tmp_fn );
}

/* ������� ��������� ����� �����������. */
int
process (char *file_name)
{

  /* ���������� ��� �������� �������������� �������� ��������.
   * ������������ ��� ����� ��������� �������� � ��������� ��������. */
  pid_t pid;

  /* ����-��������. */
  FILE *input_file = NULL;

  /* ��������� ����� ����������. */
  struct output_ctx *outctx = NULL;
  
  /* ���������� ��� ����������� ������ ����������� �����
   * ����������.*/
  FILE *outpipe = NULL;		/* ����� ��� ����� � ��������. */

  /* ��������� ����������. */
  char f_cmd[MAXLINE];		/* �������� ��� ������ �������. */
  int filter_count;		/* ���������� ��������. */

  /* ���������� ��� ������ � ��������������� �������������. */
  int c0, c, cN;	/* Counter for colors */

  /* ���������� ��� ������ � ������ TIFF �����. */
  /* ��� �������� �������� �����. */
  uint16 phm;	/* TIFF TAG *PHOTOMETRIC* */
  tsample_t spp;	/* TIFF TAG *SAMPLESPERPIXEL* */
  uint16 r_unit;	/* TIFF TAG *RESOLUTIONUNIT* */
  uint16 tiff_planar;	/* �������� TIFF PlanarConfiguration */

  TIFF *tif = NULL;	/* ��������� �� �������� TIFF. */
  char *buf = NULL;     /* ����� ��� �������� ������ ��������. */
  
  /* ��������� ����������� ����� �����������. */
  char thumbnail_name[MAXLINE];	/* ��� ����� ����������� �����. */
  TIFF *thumbnail = NULL;	/* ��������� �� �������� ����������� �����. */
  char *thumbnail_buf = NULL;	/* ����� ��� �������� ������ ��������. */
  uint32 thumbnail_width;	/* ������ ����������� �����. */
  uint32 thumbnail_height;	/* ������ ����������� �����. */
  


  /* ���������� ��� ������������ ����������� �����. */
  double thumbnail_syf;		/* ������������ ����������. */
  double thumbnail_step;	/* ������������ ���. */
  int thumbnail_sy;		/* ����� ������ � �������� �����������. */
  int thumbnail_y;		/* ����� ������ � ����������� �����. */

  /* ��������� ��������� �����������. */
  size_t ss;		/* ������ ������� � ������. */
  size_t rd;		/* ���������� ���������� ����. */
  int y;		/* ����� ������ ��������. */

  /* ���������� ��� ���������� ������� ��������� ��������� �����������. */
  int hd;		/* �ޣ���� ������� ����� ������ �����������. */
  int yd, dc;		/* ������ ������� �����. */

  /* �ޣ���� */
  int i;

  /* ��������������� ������� ��� ������������ ������� ������
   * � ������ ���������� ���������� ���������. */
  void cleanup() {
	/* �������� �������� �����. */
	if (input_file != NULL && input_file != stdin)
		if (fclose(input_file) && want_verbose)
			fprintf(stderr, "Failed to close the input file\n");
  	/* �������� ��������� �����. */
	if ( outctx ) {
		close_output( outctx );
		outctx = NULL;
	}
  	/* �������� ����������������� ������. */
	if (outpipe != NULL)
		if (pclose(outpipe) && want_verbose)
			fprintf(stderr, "Filter error\n");
	/* ������������ ������ �����������. */
	if (buf != NULL)
		_TIFFfree(buf);
	/* �������� ����� TIFF. */
	if (tif != NULL)
		TIFFClose(tif);
	/* ������������ ������ ����������� �����. */
	if (thumbnail_buf != NULL)
		_TIFFfree(thumbnail_buf);
	/* �������� ����� TIFF ����������� �����. */
	if (thumbnail != NULL)
		TIFFClose(thumbnail);

	/* �������� ��������� ������, ��������� ���������. */
	for (c = c0; c <= cN; c++) {
	  for (i = 0; i < filter_count; i++) {
		  delete_temporary_filter_file( "ct", pid, i, c );
		  delete_temporary_filter_file( "s", pid, i, c );
		  delete_temporary_filter_file( "m", pid, i, c );
	  }
	}
  }

  /* ������������� ������� ��������� �������. */
  init_cleanup(NULL);
  push_cleanup(cleanup);

  /* ���������� � ��������� �����������. �������� ������.
   * ��������� ���������������� ������� ����� ����������. */

  /* ������ ����� � ������ ��������� ����� ������� ������
   * (����� ���������� ���������������). */
  if (want_verbose)
	  fprintf(stderr, "Engrave "VERSION"\n");

  /* �������� �� �������� ���������� ����������� � ��� ������,
   * ���� ������ ����� ��������� ����������������� ������. */
  if (is_raw) {
	  if (!width || !height || !hres || !vres) {
		  fprintf(stderr, "It is necessary to specify WIDTH, HEIGHT, HRES & VRES to process a RAW image stream (file).\n");
		  exit(EXIT_FAILURE);
	  }
  }

  /* ���� ��������� ���������� ��������������� �����������
   * � ������� TIFF, �� ���������� ������� ��������� ����
   * � ��������������� ������������. */
  if (!is_raw) {
	  tif = open_tif_file(file_name, &phm, &spp, &r_unit, &tiff_planar);
  } else if (file_name != NULL && strlen(file_name) > 0) {
	  /* ����� ������������ ������� �������� ����� � ������������������
	   * ������� ��� ������������ �������� ������, ���� ��� ����� ��
	   * �������. */
	  input_file = fopen(file_name, "r");
  } else {
	  input_file = stdin;
  }

  /* ��������� ��������� �����. */
  if ( prepare_output( file_name, output_name, &outctx) != 0 )
  {
	  exit(EXIT_FAILURE);
  }

  /* ��������, ������ �� ���� �� ���� ������? */
  if (filter[0] == '\0') {
	  fprintf(stderr, "It is necessary to specify filter(s) to process image through.\n");
	  exit(EXIT_FAILURE);
  }
  
  /* ���������� � ���������� �����������. */
  
  /* ��������� �������������� �������� ��������. */
  pid = getpid();
  
  /* ������ ���������� ����� ��������. */
  parse_filters(f_cmd, &filter_count, pid);

  /* ������ ��������������� ����������
   * (����� ���������� ���������������). */
  if (want_verbose) {
	  if (is_cmyk)
		  fprintf(stderr, "Processing CMYK image:\n");
	  else
		  fprintf(stderr, "Processing grayscale image:\n");
	  fprintf(stderr, "Width: %u\nHeight: %u\nHRes: %.2f\nVRes: %.2f\n", width, height, hres, vres);
	  fprintf(stderr, "Filters: %s\n", f_cmd);
  }

  /* ������ ��������� �������� � �������� �����������������
   * ������ � ����. */
  outpipe = popen(f_cmd, "w");
  /* ������ ��������� �� ������ � ����� � ������ �������. */
  if (outpipe == NULL) {
	  fprintf(stderr, "Can't create pipe attached to a filter process\n");
	  exit(EXIT_FAILURE);
  }

  /* ��������� ���������� ��������� ��������
   * � ����������� � ����������� �����������. */
  if (is_cmyk) {
	  ss = 4;	/* 4 ����� �� ������� ��� 4 ���������� �����������. */
	  c0 = 0;	/* 0 ��� ������ ������� ���������. */
	  cN = 3;	/* 3 ��� ������ ���������� ���������. */ 
  } else {
	  ss = 1;	/* 1 ���� ��� �������� �����������. */
	  c0 = 3;	/* 3 ��� ������ ������� */
	  cN = 3;	/* � ���������� ���������. */
  }

  /* ���������� � ��������� ����� �����������. */

  /* ��������� ������ ��� �������� ����� ��������� �����������. */
  buf = _TIFFmalloc(ss*width);
  /* ������ ��������� �� ������ � ����� � ������ �������. */
  if (buf == NULL) {
	  fprintf(stderr, "Scanline buffer allocation failed\n");
	  exit(EXIT_FAILURE);
  }

  /* �������� ����� ����������� ����� ����������� �� ������.
   * ��������� ������. */
  if (want_preview) {
  	  /* ���������� ����� ����������� ����������� �����. */
	  prepare_preview(thumbnail_name, pid, phm, ss, tiff_planar, &thumbnail, &thumbnail_buf, &thumbnail_width, &thumbnail_height);

	  /* ����������� ���������� �����������.
	   * ����������� ������� ��������� ����������. */
	  thumbnail_step = height/(double)thumbnail_height;
	  thumbnail_syf = 0;
	  thumbnail_sy = 0;
	  thumbnail_y = 0;
  }

  /* ������ ���������� ��������� (����� ���������� ���������������) */
  if (want_verbose)
	  fprintf(stderr, "Progress: ");

  /* ���������������� ������ ����� ��������� �����������.
   * ������ ������������ � �����, ������ ���������� ��
   * �������������� ������ �� ����������. */

  /* ���������� ���������� ����� � ������� ����� �����������. */
  hd = height/10;

  /* ��������� �ޣ������. */
  yd = 0;
  dc = 0;
  for (y = 0; y < height; y++) {
	  
	  if (is_raw) {
	  	  /* ������������������ ������ �������� �� ������������
		   * �������� ������. */
		  rd = fread(buf, ss, width, stdin);
		  /* �������� ������������ ���������� ����������� ����. */
		  if (rd < width) {
			  fprintf(stderr, "Image stream suddenly closed.\n");
			  exit(EXIT_FAILURE);
		  }
	  } else
	  	  /* ������ ������ ������ ����������� �� TIFF �����. */
		  TIFFReadScanline(tif, buf, y, 0);

	/* ���� ������� ������ ������ ���� �������� � ����������� �����,
	 * �� ������������ ţ ��������������� � ������ �� ��������� ����
	 * ����������� �����. */
	  if (thumbnail != NULL && y == thumbnail_sy) {
		  thumbnail_syf += thumbnail_step;
		  thumbnail_sy = rint(thumbnail_syf);
		  get_thumbnail_line(buf, ss, width, thumbnail_buf, thumbnail_width);
		  TIFFWriteScanline(thumbnail, thumbnail_buf, thumbnail_y++, 0);
	  }

	  /* ������ ������ ������ ����������� � ����������������
	   * �����. ���� ������� ��������� ����������� ��������� �������
	   * � �������� ������ �����������, �� ����������� �������������
	   * ��������. */
	  rd = fwritesmp(buf, ss, width, outpipe, miniswhite && want_intensity || !miniswhite && want_density, NULL);
	  if (rd < width) { /* ��������� ��������� ��������. */
		  fprintf(stderr, "Failed to transfer scanline data further\n");
		  exit(EXIT_FAILURE);
	  }
	 
	  /* ���������� ������� ���������. */
	  yd++;
	  if (yd > hd) {
		  dc++;
		  yd = 0;
		  if (want_verbose)
			  fprintf(stderr, "%i0%% ",  dc);
	  }
  }

  /* �������� ����������������� ������ ����� ������ ���� �����. */
  if (outpipe != NULL) {
	  if (pclose(outpipe)) {
		  if (want_verbose)
			  fprintf(stderr, "Filter error\n");
		  outpipe = NULL;
		  exit(EXIT_FAILURE);
	  }
	  outpipe = NULL;
  }

  /* ������ ���������� 100% (����� ���������� ���������������). */
  if (want_verbose)
	  fprintf(stderr, "100%\n");

  if ( outctx ) {
	  /* ��������� ������, ��������� � ���������� ������ ��������. */
	  if ( outformat == EPS_FMT ) {
	  /* ����� ��������� PostScript-��������� � ������������ ������. */
		  ps_header( file_name, outctx->output_file );
	  }
	  
	  /* ���� �����������, ��������� ������ �� �������� ������������ ������.
	   * ��������������� �������������� �����, ���������� ���������� ���
	   * ������� �� ����������. */
	  for (c = c0; c <= cN; c++) {
		  /* ���� ����ޣ� ����� ������ ��������� ����������, �� ������������
		   * �������� ������ �������� ���������.*/
		  if (c0 == 0 && (want_c || want_m || want_y || want_k)) {
			  /* ����  ������� ��������� �� ��� ������������� ������, �� ��
				 ������������. */
			  if (c == 0 && !want_c) {
				  continue;
			  }
			  if (c == 1 && !want_m) {
				  continue;
			  }
			  if (c == 2 && !want_y) {
				  continue;
			  }
			  if (c == 3 && !want_k) {
				  continue;
			  }
		  }

		  if ( outformat == EPS_FMT ) {
			  fprintf( outctx->output_file,
					  "%% Painting '%s' image color\n",
					  get_cmykcolor_name(c) );
			  /* � ��������� ������������ �������� ��� ������ ������
			   * ������ � ����� ����������. */
			  fprintf( outctx->output_file, "%s\n", get_cmyk_color(c) );
			  fprintf( outctx->output_file, "%% Painting CT images\n" );
		  }
		  
		  /* ��������� �������� ����������� �� ������� �� ��������. */
		  for (i = 0; i < filter_count; i++) {
			  use_temporary_filter_file( outctx, "ct", pid, i, c );
		  }

		  if ( outformat == EPS_FMT ) {
			  fprintf( outctx->output_file, "%% Painting mask images\n" );
		  }
		  
		  /* ��������� ������������ ����������� �� ������� �� ��������. */
		  for (i = 0; i < filter_count; i++) {
			  use_temporary_filter_file( outctx, "m", pid, i, c );
		  }

		  if ( outformat == EPS_FMT ) {
			  fprintf( outctx->output_file, "%% Painting stroke images\n" );
		  }
		  
		  /* ��������� ��������� ����������� �� ������� �� ��������. */
		  for (i = 0; i < filter_count; i++) {
			  use_temporary_filter_file( outctx, "s", pid, i, c );
		  }
	  }

	  if ( outctx && outformat == EPS_FMT ) {
		  /* ������ ����������� ����� PostScript ���������. */
		  fprintf( outctx->output_file,
				   "end\n"
				   "grestore\t%% Restore previous graphic state\n"
				   "showpage\n"
				   "%%%%EOF\n");
	  }
  
  }

  /* ��������� �������� ������ ���������� � ����������� �����. */
  int to_stdout = ( outctx && outctx->output_file == stdout);

  /* ���������� ��������� ������������ ������� ��������. */
  (*pop_cleanup())();

  /* ���� ������ ��������������� �������, �� ������������ ���������� �
   * PostScript-����� ����������� ����� ���������� � ������� TIFF. */
  if (want_preview && !to_stdout ) {
	  if ( outformat == EPS_FMT ) {
		  /* ���������� ����������� ����� � PostScript-�����. */
		  add_preview(output_name, thumbnail_name);
	  }
  }

  return 0;
}
