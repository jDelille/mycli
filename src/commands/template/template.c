#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <ctype.h>
#include <stdbool.h>

#include "../utils/defs.h"

/* Check if a file exists */
bool file_exists(const char *path)
{
    return access(path, F_OK) == 0;
}

/* Convert Windows-style paths to WSL/Linux-style paths. (C:\...) --> (/mnt/c/...) */
void convert_windows_path_to_wsl(char *path)
{
    if (strlen(path) >= 3 && path[1] == ':' && (path[2] == '\\' || path[2] == '/'))
    {
        char temp[512];
        snprintf(temp, sizeof(temp), "/mnt/%c/%s", tolower(path[0]), path + 3);
        strncpy(path, temp, 512);
    }
    // Replace backslashes with forward slashes
    for (char *p = path; *p; ++p)
        if (*p == '\\')
            *p = '/';
}

/* Ensure that the .templates directory exists. if not then create */
bool ensure_templates_dir(void)
{
    return access(".templates", F_OK) == 0 || mkdir(".templates", 0700) == 0;
}

/* Extract the filename from a path */
char *get_filename(const char *path)
{
    char *copy = strdup(path);
    char *name = basename(copy);
    char *result = strdup(name);
    free(copy);
    return result;
}

/* Copy a file from src to dst */
bool copy_file(const char *src, const char *dst)
{
    FILE *in = fopen(src, "r");
    if (!in)
    {
        perror("Error opening source");
        return false;
    }
    FILE *out = fopen(dst, "w");
    if (!out)
    {
        perror("Error creating dest");
        fclose(in);
        return false;
    }

    char buf[1024];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0)
        fwrite(buf, 1, n, out);

    fclose(in);
    fclose(out);
    return true;
}

/* Install a template into the .templates directory */
void install_template(const char *templatePath)
{
    char path[512];
    snprintf(path, sizeof(path), "%s", templatePath);
    convert_windows_path_to_wsl(path);
    if (!ensure_templates_dir())
        return;

    if (!file_exists(path))
    {
        printf("Error: The file %s does not exist\n", path);
        return;
    }

    char *filename = get_filename(path);
    char dest[512];
    snprintf(dest, sizeof(dest), ".templates/%s", filename);
    free(filename);

    if (file_exists(dest))
    {
        printf("Template already exists: %s\n", dest);
        return;
    }
    if (copy_file(path, dest))
        printf("Installed template: %s\n", dest);
}

/* Look up a placeholder's replacement value */
const char *get_placeholder(const char *name, const char *keys[], const char *vals[], int n)
{
    for (int i = 0; i < n; i++)
        if (strcmp(name, keys[i]) == 0)
            return vals[i];
    return "";
}

/* Create directory from line */
void create_dirs_from_line(const char *line, const char *projectName)
{
    // Skip leading slashes and spaces
    const char *dirName = line;
    while (*dirName == '/' || *dirName == ' ')
        dirName++;

    // Skip empty lines
    if (*dirName == '\0')
        return;

    size_t len = strlen(dirName);
    if (dirName[len - 1] != '/')
        return; // Not a directory, skip

    char path[512];
    snprintf(path, sizeof(path), "%s/%.*s", projectName, (int)(len - 1), dirName); // remove trailing /

    mkdir(path, 0755);
}

/* Create file from line */
void create_file_from_line(FILE *templateFile, char *path, const char **placeholders, const char **replacements, int n)
{
    char line[1024];
    char buffer[4096];

    // Remove trailing newline and whitespace from path
    size_t len = strlen(path);
    while (len > 0 && isspace((unsigned char)path[len - 1]))
        path[--len] = '\0';

    // Ensure parent directories exist
    char tmp[512];
    strcpy(tmp, path);
    char *dir = dirname(tmp);
    if (dir)
        mkdir(dir, 0755);

    FILE *out = fopen(path, "w");
    if (!out)
    {
        perror("File create failed");
        return;
    }

    // Write file content until next file/directive
    while (fgets(line, sizeof(line), templateFile))
    {
        if (strncmp(line, "// ", 3) == 0)
        {
            fseek(templateFile, -strlen(line), SEEK_CUR); // Backtrack to handle next file
            break;
        }

        buffer[0] = 0;

        // Replace placeholders
        const char *p = line;
        while (*p)
        {
            char *start = strchr(p, '[');
            if (!start)
            {
                strcat(buffer, p);
                break;
            }

            strncat(buffer, p, start - p);
            char *end = strchr(start, ']');
            if (!end)
            {
                strcat(buffer, start);
                break;
            }

            char ph[128];
            strncpy(ph, start + 1, end - start - 1);
            ph[end - start - 1] = 0;

            const char *rep = "";
            for (int i = 0; i < n; i++)
                if (strcmp(ph, placeholders[i]) == 0)
                    rep = replacements[i];

            strcat(buffer, rep);
            p = end + 1;
        }

        fputs(buffer, out);
    }

    fclose(out);
}

/* Generate a new file from a template */
void generate_project_from_template(const char *templateName, const char *projectName)
{
    /* Locate the template */
    char templatePath[512];
    snprintf(templatePath, sizeof(templatePath), ".templates/%s", templateName);

    FILE *templateFile = fopen(templatePath, "r");
    if (!templateFile)
    {
        printf("Template open failed: %s\n", templatePath);
        return;
    }

    /* Setup variables */
    char line[1024];

    const char *placeholders[] = {"PROJECT_NAME"};
    const char *replacements[] = {projectName};
    int n = 2;

    mkdir(projectName, 0755);

    /* Read the template line by line */
    while (fgets(line, sizeof(line), templateFile))
    {

        /* Ignore comments */
        if (strncmp(line, "// #", 4) == 0)
        {
            continue;
        }

        /* Handle files */
        if (strncmp(line, "//", 2) == 0)
    {
        // Determine if line is a directory or file
        size_t len = strlen(line);
        if (line[len - 2] == '/') // trailing slash before newline
            create_dirs_from_line(line + 2, projectName);
        else
        {
            char currentFile[512];

            snprintf(currentFile, sizeof(currentFile), "%s/%s", projectName, line + 3);

            currentFile[strcspn(currentFile, "\n")] = 0;
            
            create_file_from_line(templateFile, currentFile, placeholders, replacements, n);
        }
    }

        /* Handle directories */
        else if (strncmp(line, "//", 3) == 0)
        {
            create_dirs_from_line(line + 2, projectName);
        }
    }

    printf("Project %s%s%s created using the %s%s%s template\n",
           STYLE_GREEN, projectName, STYLE_RESET,
           STYLE_GREEN, templateName, STYLE_RESET);

    fclose(templateFile);
}