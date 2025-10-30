#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include "scaffold.h"
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../template/template.h"
#include "../utils/defs.h"
#include "../utils/selections.h"

/* List templates */
int list_templates(char templates[][256], int max_templates)
{
    DIR *d = opendir(".templates");
    if (!d)
        return 0;

    struct dirent *file;
    struct stat file_info;

    int count = 0;
    char path[512];

    while ((file = readdir(d)) != NULL && count < max_templates)
    {
        snprintf(path, sizeof(path), ".templates/%s", file->d_name);

        if (stat(path, &file_info) == 0 && S_ISREG(file_info.st_mode))
        {
            strncpy(templates[count], file->d_name, 256);
            templates[count][255] = '\0';
            count++;
        }
    }

    closedir(d);
    return count;
}

/* Step 1: Project Name */
void get_name(char *project_name, size_t size)
{
    printf("%s What is your project named? » ", ICON_QUESTION);
    fflush(stdout);

    fgets(project_name, size, stdin);
    project_name[strcspn(project_name, "\n")] = 0;

    CLEAR_PREV_LINE();

    printf("%s What is your project named? » %s\n", GREEN_CHECK, project_name);
}

/* Step 2: Project Template */
void choose_template(char *template_name, size_t size)
{
    char templates_list[64][256];
    int num_templates = list_templates(templates_list, 64);

    if (num_templates == 0)
    {
        printf("No templates found in .templates\n");
        template_name[0] = '\0';
        return;
    }

    const char *template_names[64];
    for (int i = 0; i < num_templates; i++)
        template_names[i] = templates_list[i];

    int selected = selection("Which template do you want to use?", template_names, num_templates);

    strncpy(template_name, templates_list[selected], size);
    template_name[size - 1] = '\0';
}

void scaffold()
{
    char project_name[256];
    char template_name[256];

    get_name(project_name, sizeof(project_name));
    choose_template(template_name, sizeof(template_name));
    
    generate_project_from_template(template_name, project_name);
}