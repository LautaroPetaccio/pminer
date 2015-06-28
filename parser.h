#ifndef _PARSER_H
#define _PARSER_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

/* Arguments*/
struct arguments {
	int verbose_flag;
	char *host;
	char *port;
	char *username;
	char *password;
};

void parse_arguments(int argc, char **argv, struct arguments *arguments);
void print_usage();

#endif /* parser.h */


