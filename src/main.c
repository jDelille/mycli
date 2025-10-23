#include <stdio.h>
#include "cli.tab.h"   // Bison-generated header
#include "commands/commands.h"

int yyparse(void);

int main(void) {
    printf("CLI started. Type commands (e.g., create project myproj):\n");
    yyparse();  // This runs the parser and triggers your create_project action
    return 0;
}