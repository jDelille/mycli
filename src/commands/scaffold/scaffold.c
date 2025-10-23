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

/* Clean input buffer */
void flush_stdin()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

/* Read a keypress without waiting for Enter */
int getch()
{
    struct termios oldt, newt;
    int ch;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

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

        if (stat(path, &file_info) == 0 && S_ISREG(file_info.st_mode)) // S_ISREG checks if a file is a regular file
        {
            strncpy(templates[count], file->d_name, 256);
            templates[count][255] = '\0';
            count++;
        }
    }

    closedir(d);
    return count;
}

/* Yes or no option */
int choose_yes_no_horizontal(const char *prompt)
{
    const char *options[] = {"Yes", "No"};
    int num_options = 2;
    int selected = 0;
    int ch;

    printf("%s %s » ", ICON_QUESTION, prompt);
    fflush(stdout);

    while (1)
    {
        for (int i = 0; i < num_options; i++)
        {
            (i == selected)
                ? printf("%s%s%s", STYLE_BLUE_UNDERLINE, options[i], STYLE_RESET)
                : printf("%s", options[i]);

            if (i < num_options - 1)
                printf(" / ");
        }

        fflush(stdout);

        ch = getch();

        if (ch == KEY_ESC && getch() == KEY_BRACKET)
        {
            int arrow = getch();
            if (arrow == KEY_ARROW_RIGHT && selected < num_options - 1)
                selected++;
            if (arrow == KEY_ARROW_LEFT && selected > 0)
                selected--;
        }
        else if (ch == KEY_ENTER)
        {
            printf("\r\033[2K");
            printf("%s %s » %s\n", GREEN_CHECK, prompt, options[selected]);
            return selected == 0;
        }
        printf("\r\033[2K%s %s » ", ICON_QUESTION, prompt);
    }
}

/* Step 1: Project Name */
void get_name(char *project_name, size_t size)
{
    printf("%s What is your project named? » ", ICON_QUESTION);
    fflush(stdout);

    fgets(project_name, size, stdin);
    project_name[strcspn(project_name, "\n")] = 0;

    // Move cursor up 1 line
    printf("\033[1A"); // Clear the entire line
    printf("\033[2K");
    // Print the updated line
    printf("%s What is your project named? » %s\n", GREEN_CHECK, project_name);
}

/* Step 2: Project Template */
void choose_template(char *template_name, size_t size)
{
    char templates[64][256];
    int num_templates = list_templates(templates, 64);
    if (num_templates == 0)
    {
        printf("No templates found in .templates\n");
        template_name[0] = 0;
        return;
    }

    int selected = 0;
    int ch;

    printf("%s Which template do you want to use? %s- Use arrow keys. Return to submit.%s\n",
       ICON_QUESTION, STYLE_GRAY, STYLE_RESET);
    HIDE_CURSOR();

    while (1)
    {
        for (int i = 0; i < num_templates; i++)
        {
            if (i == selected)
                PRINT_HIGHLIGHTED(templates[i]);
            else
                PRINT_NORMAL(templates[i]);
        }

        ch = getch();

        if (ch == KEY_ESC && getch() == KEY_BRACKET)
        {
            int arrow = getch();
            if (arrow == KEY_ARROW_UP && selected > 0)
            {
                selected--;
            }

            if (arrow == KEY_ARROW_DOWN && selected < num_templates - 1)
            {
                selected++;
            }
        }
        else if (ch == 10)
        {
            SHOW_CURSOR();
            strncpy(template_name, templates[selected], size);
            template_name[size - 1] = '\0';

            MOVE_CURSOR_UP(num_templates + 1);

            // Clear all lines: prompt + list
            for (int i = 0; i < num_templates + 1; i++)
            {
                CLEAR_LINE();

                if (i < num_templates)
                {
                    MOVE_CURSOR_DOWN(1);
                }
            }

            MOVE_CURSOR_UP(num_templates);

            printf("%s Which template do you want to use? » %s\n", GREEN_CHECK, template_name);

            break;
        }

        // Clear for redraw
        MOVE_CURSOR_UP(num_templates);
        for (int i = 0; i < num_templates; i++)
        {
            CLEAR_TO_EOL_PRINTLN();
        }
        MOVE_CURSOR_UP(num_templates);
    }

    SHOW_CURSOR();
}

void scaffold()
{
    char project_name[256];
    char template_name[256];

    get_name(project_name, sizeof(project_name));

    choose_template(template_name, sizeof(template_name));
    generate_project_from_template(template_name, project_name);
}