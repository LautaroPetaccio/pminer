#include "parser.h"

void print_usage() {
	printf("PMiner - bitcoin miner usage: \n");
	printf("-h hostname:port \n");
	printf("-u username \n");
	printf("-p password \n");
}

void parse_arguments(int argc, char **argv, struct arguments *arguments) {
  /* Default values */
  arguments->verbose_flag = 0;
  arguments->host = 0;
  arguments->port = "3334";
  arguments->username = 0;
  arguments->password = 0;

  int c;
  while(1) {
	static struct option long_options[] =
          {
            /* These options set a flag. */
            {"verbose", no_argument, 0, 1},
            /* These options don’t set a flag.
               We distinguish them by their indices. */
            {"host",  required_argument, 0, 'h'},
            {"username",    required_argument, 0, 'u'},
            {"password",    required_argument, 0, 'p'},
            {0, 0, 0, 0}
          };
        /* getopt_long stores the option index here. */
    int option_index = 0;

    c = getopt_long (argc, argv, "h:u:p:",
                     long_options, &option_index);

    /* Detect the end of the options. */
    if (c == -1)
      break;
    switch (c) {
      case 0:
        /* If this option set a flag, do nothing else now. */
        if (long_options[option_index].flag != 0)
          break;
        /* printf ("option %s", long_options[option_index].name); */
        if (optarg)
          printf (" with arg %s", optarg);
        printf ("\n");
        break;

      case 'h':
        /* printf ("option -h with value `%s'\n", optarg); */
        arguments->host = strtok(optarg, ":");
        char *port = strtok(NULL, ":");
        if(!port) {
        	printf("Port not found, using 3334 \n");
        }
        else {
        	arguments->port = port;
        }
        /* printf("Host: %s and Port: %s \n", arguments->host, arguments->port); */
        break;

      case 'u':
        /* printf ("option -u with value `%s'\n", optarg); */
        arguments->username = optarg;
        break;

      case 'p':
        /* printf ("option -p with value `%s'\n", optarg); */
        arguments->password = optarg;
        break;

      default:
      	/* Ignore unkown parameters */
      	break;
      }
  }
}