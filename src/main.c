#include <string.h>
#include "parser.h"

/* program name */
static char *program_name  = NULL;


int main(int argc, char **argv)
{
	char *ptr;

	/* save the program name in a static variable */
	if ((ptr = strrchr(argv[0], '/')) != NULL) {
		program_name = ++ptr;
	} else {
		program_name = argv[0];
	}
	
	parse_arguments(argc,argv);

	return 0;
}



const char *get_program_name()
{
	return program_name;
}



