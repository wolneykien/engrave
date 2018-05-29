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


/* ���������� �������, ������������ ��� ���������� ��������. */

#include <stdio.h>
#include <sys/types.h>
#include "system.h"
#include "filter.h"
#include "misc.h"

/* ��� �������������� ��������. */
char *program_name;

/* ����� ������������ �������. */
pid_t pid;

/* ���������� ����� �������. */
int fidx;

/* ������� ������ ���������� ���������������. */
int want_verbose;

/* �������� ���������� */

/* ��������� �����������: */
unsigned long int width;	/* ������ �����������; */
int unsigned long height;	/* ������ �����������; */
float hres;             	/* ���������� �� �����������; */
float vres;			/* ���������� �� ���������; */
int is_cmyk;			/* ������� 4-���������� �����������; */
int miniswhite;			/* ������� ����������� �����������. */

/* ����������� ������� ���������� ��������� ������ ��� ���� ��������. */
static struct option const base_long_options[] =
{
	{"help", no_argument, NULL, 'H'},
	{"version", no_argument, NULL, 'V'},
	{"pid", required_argument, NULL, 'p'},
	{"index", required_argument, NULL, 'i'},
	{"width", required_argument, NULL, 'w'},
	{"height", required_argument, NULL, 'h'},
	{"hres", required_argument, NULL, 'x'},
	{"vres", required_argument, NULL, 'y'},
	{"cmyk", no_argument, NULL, 'c'},
	{"density", no_argument, NULL, 'D'},
	{"intensity", no_argument, NULL, 'I'},
	{"verbose", no_argument, NULL, 'v'},
	{NULL, 0, NULL, 0}
};

/**
 * ���������� ��� ���������� ����� ��� ������ ���� �����������
 * � ������, ��������������� ��������� ����������: ��������
 * ������� #fsuf � ������ ��������� ������ #color_idx.
 */
const char *
get_tmp_filter_file_name( const char *fsuf, int color_idx )
{
	if ( !pid ) {
		fprintf( stderr, "%s: Can't create temp. file: parent PID isn't set\n",
				 program_name );
		return NULL;
	}
	
	/* �������� ������ ��� ����� �� ������ ��������, ������ ������� �
	 * ������ ��������� ������.
	 */
	return get_tmp_file_name( fsuf, pid, fidx, color_idx );
}

/* ����� ������� ������� � ���������� ������ � ��������� ����� ��������. */
void
usage (int status, usage_header_f usage_header, usage_params_f usage_params)
{
  
  if (usage_header != NULL) {
	usage_header(stdout);
  }

  printf (_("\n\
\n\
Copyright (C) 2007 Yuri V. Kuznetsov, Paul Wolneykien.\n\
\n\
This program is free software: you can redistribute it and/or modify\n\
it under the terms of the GNU General Public License as published by\n\
the Free Software Foundation, either version 3 of the License, or\n\
(at your option) any later version. See http://www.gnu.org/licenses/\n\
for details.\n\n"));
  printf (_("Usage: %s [OPTION]...\n"), program_name);
  printf (_("\
Options:\n\
  -p PID, --pid=PID		PID of the parent process\n\
  -i IDX, --index=IDX		filter index\n\
  -w WIDTH, --width=WIDTH	RAW image width\n\
  -h HEIGHT, --height=HEIGHT	and height\n\
  -x HRES, --hres=HRES		RAW image horizontal\n\
  -y VRES, --vres=VRES		and vertical resolution\n\
  -c, --cmyk			RAW image is CMYK\n\
  -D, --density			input and output data is DENSITY \n\
                                values\n\
  -I, --intensity		intput and output data is INTENSITY\n\
                                values\n\
  -H, --help			display this help and exit\n\
  -V, --version			output version information and exit\n\
"));

  if (usage_params != NULL) {
	usage_params(stdout);
  }

  printf (_("\n"));

  exit (status);
}

/* ����������� ���������� ���������� ���������� ������, ������̣���� �
 * �������. */
int
options_count(struct option const *opts)
{

	int c = 0;
	while (opts->name != NULL || opts->has_arg != 0
		|| opts->flag !=  NULL || opts->val != 0) {
		opts++;
		c++;
	}

	return c;

}

/* ����������� ������� ������� ���������� ���������� ������ ��� �������.
 * ����������� ��������� ���������� � �������������� ������� �ͣ�.
 */
int
decode_switches (int argc, char **argv, int error_code,
		 struct option const *long_options,
		 char ***option_vars,
		 usage_header_f usage_header, usage_params_f usage_params)
{
  int c;
  char *endptr;
  int option_index;

  /* ������� �������� ��������� ��������. */
  int want_intensity = 0;

  /* ������� �������� �������� ���������� ���������. */
  int want_density = 0;

  /* ������ ��������, ������������ ����� ������� � ��������������
   * ���������� ���������� ������ ��������������. */
  int base_options_count;
  int special_options_count;

  /* ����� ��� �������� �������� ���������� ���������� ������. */
  struct option *all_options = NULL;

  /* ��������� �������� �� ���������.  */
  is_cmyk = 0;
  miniswhite = 0;
  width = 0;
  height = 0;
  vres = 0;
  hres = 0;
  pid = 0;
  fidx = 0;
  want_verbose = 0;

  /* ����ޣ� ���������� ������� ����������. */
  base_options_count = options_count(base_long_options);

  /* ����ޣ� ���������� �������������� ����������. */
  special_options_count = options_count(long_options);

  /* ������� ��������� ������� ������� ��������. */
  void cleanup() {
	if (all_options != NULL) {
		free(all_options);
	}
  }

  /* ������������� ������� ��������� �������. */
  push_cleanup(cleanup);

  /* ��������� ������ ��� ����������� �������� �������� ����������. */
  all_options = calloc(base_options_count + special_options_count + 1, sizeof(struct option));

  /* ����������� �������� ������� ���������� � �����. */
  memcpy(all_options, base_long_options, base_options_count*sizeof(struct option));
  /* ������ ���������� �������� ����������� ����������. */
  memcpy(all_options + base_options_count, long_options, (special_options_count + 1)*sizeof(struct option));

  /* ���������������� ������� ������� ����������. */
  while ((c = getopt_long (argc, argv,
		"p:"  /* ����� ��������� ������������ ��������; */
	   	"i:"  /* ����� �������; */
		"w:"  /* ������ �����������; */
		"h:"  /* ������ �����������; */
		"x:"  /* ���������� �� �����������; */
		"y:"  /* ���������� �� ���������; */
		"c"  /* ������� 4-���������� �����������; */
		"D"	/* ������� �������� �������� ���������; */
		"I"	/* ������� �������� ��������� ��������; */
		"H"	/* ����� ������� �������; */
		"v"	/* ����� ���������� ���������������; */
		"V",	/* ����� ���������� � ������. */
		all_options, &option_index)) >= 0)
    {
      /* ������������ ��������� �� �����. */
      switch (c)
	{
		
	/* ����� ��������� ������������ ��������. �������������� ����������
	 * ������������� � ��������. �����, � ������ ������ ��������������.
         */
	case 'p':
		pid = strtoul(optarg, &endptr, 0);
		if (errno == ERANGE || *endptr != '\0') {
			fprintf(stderr, "%s: PID value is invalid.\n", program_name);
			exit(error_code);
		}
		break;

	/* ������ �����������. �������������� ����������  ������������� � 
	 * ��������. �����, � ������ ������ ��������������.
	 */
	case 'w':
		width = strtoul(optarg, &endptr, 0);
		if (errno == ERANGE || *endptr != '\0') {
			fprintf(stderr, "%s: Width value is invalid.\n", program_name);
			exit(error_code);
		}
		break;
	
	/* ������ �����������. �������������� ����������  ������������� � 
	 * ��������. �����, � ������ ������ ��������������.
	 */	
	case 'h':
		height = strtoul(optarg, &endptr, 0);
		if (errno == ERANGE || *endptr != '\0') {
			fprintf(stderr, "%s: Height value is invalid.\n", program_name);
			exit(error_code);
		}
		break;

	/* ���������� �� �����������. �������������� ����������  ������������� � 
	 * ��������. �����, � ������ ������ ��������������.
	 */
	case 'x':
		hres = strtod(optarg, &endptr);
		if (errno == ERANGE || *endptr != '\0') {
			fprintf(stderr, "%s: HRES value is invalid.\n", program_name);
			exit(error_code);
		}
		break;

	/* ���������� �� ���������. �������������� ����������  ������������� � 
	 * ��������. �����, � ������ ������ ��������������.
	 */
	case 'y':
		vres = strtod(optarg, &endptr);
		if (errno == ERANGE || *endptr != '\0') {
			fprintf(stderr, "%s: VRES value is invalid.\n", program_name);
			exit(error_code);
		}
		break;

	/* ������� 4-���������� �����������. ���� ����� ������� �� ������
	 * ������� ��������� ��������, �� �� ����������� ������� (��������������
         * �������, ����������� 0.
	 */
	case 'c':
		is_cmyk = 1;
		if (!want_intensity)
			miniswhite = 1;
		break;

	/* ������� �������� ���������. �� ����������� ������� (��������������
         * �������, ����������� 0. �����, ���� ������������ ��� ������
	 * ��������������� �������.
	 */
	case 'D':
		if (!want_intensity) {
			miniswhite = 1;
			want_density = 1;
		} else {
			fprintf(stderr, "INTNSITY and DENSITY are mutually exclusive options\n");
			exit(error_code);
		}
		break;

	/* ������� ��������� ��������. �� ����������� ������� (��������������
         * �������, ����������� 0. �����, ���� ������������ ��� ������
	 * ��������������� �������.
	 */
	case 'I':
		if (!want_density) {
			miniswhite = 0;
			want_intensity = 1;
		} else {
			fprintf(stderr, "INTNSITY and DENSITY are mutually exclusive options\n");
			exit(error_code);
		}
		break;

	/* ����� �������. �������������� ���������� ������������� ������ �
	 * ��������. ����� � ��������� ������, � ������ ������ ��������������.
	 */
	case 'i':
		fidx = strtoul(optarg, &endptr, 0);
		if (errno == ERANGE || *endptr != '\0') {
			fprintf(stderr, "%s: INDEX value is invalid.\n", program_name);
			exit(error_code);
		}
		break;

	/* ��������� ������ ���������� ���������������. */
	case 'v':
		want_verbose = 1;
		break;
		
	/* ����� ���������� � ������ � ���������� ������. */
	case 'V':
	  printf ("%s filter from %s %s\n", program_name, PACKAGE, VERSION);
	  exit (0);

	/* ����� ������� ������� � ���������� ������. */
	case 'H':
	  usage (0, usage_header, usage_params);

        case '\0':
	  if (option_vars != NULL && option_vars[option_index - base_options_count] != NULL) {
			*option_vars[option_index - base_options_count] = optarg;
	  }
	  break;

	/* ���� ���� ��������� �� ��� ���������������, �� ������������ �����
	 * ������� ������� � ���������� ������ � ��������� ������.
	 */
	default:
	  usage (error_code, usage_header, usage_params);
	}
    }

   /* ������������ ������� ��������. */
   (*pop_cleanup())();

  /* ����������� ������ ������� �� ������������� ���������. */
  return optind;
}

/* �������� ������ ����������� �� ���������� ������, ��������� �� ���ޣ���
 * ���������� ������� � ���������� ���������� ����� ���ޣ���, � �������� �����
 * �� ���������� ���������.
 */
void
write_outbuf(char *outbuf, size_t ss, size_t len) {

	size_t wt;

	wt = fwritesmp(outbuf, ss, len, stdout, miniswhite, NULL);
	if (wt < len) {
		fprintf(stderr, "%s: Failed to transfer scanline data further\n", program_name);
		/* ����� � ��������� ������, ���� ������ ������ � �����
		 * ����������� �������.
		 */
		exit(EXIT_FAILURE);
	}

}
