#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scaffold/scaffold.h"
#include "template/template.h"

void create_project()
{
    scaffold();
}

void install_template_cmd(const char *path)
{
    printf("Installing template: %s\n", path);
    install_template(path);
}

void generate_proj_from_template(const char *templateName, const char *projectName)
{
    generate_project_from_template(templateName, projectName);
}
