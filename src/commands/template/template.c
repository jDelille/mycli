#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <ctype.h>
#include <stdbool.h>

#include "../utils/defs.h"

bool file_exists(const char *file_path)
{
    return access(file_path, F_OK) == 0;
}

void convert_windows_path_to_wsl(char *file_path)
{

    bool is_windows_path =
        strlen(file_path) >= 3 &&
        file_path[1] == ':' &&
        (file_path[2] == '\\' || file_path[2] == '/');
    ;

    if (is_windows_path)
    {
        char wsl_path[512];
        snprintf(wsl_path, sizeof(wsl_path), "/mnt/%c/%s", tolower(file_path[0]), file_path + 3);
        strncpy(file_path, wsl_path, 512);
    }

    for (char *current_char = file_path; *current_char; ++current_char)
    {
        if (*current_char == '\\')
        {
            *current_char = '/';
        }
    }
}

bool ensure_template_dir_exists(void)
{
    return access(".templates", F_OK) == 0 || mkdir(".templates", 0700) == 0;
}

char *get_filename(const char *path)
{
    char *path_copy = strdup(path);
    char *base_name = basename(path_copy);
    char *result = strdup(base_name);
    free(path_copy);
    return result;
}

/* Copy a file from src to dst */
bool copy_file(const char *src_path, const char *dst_path)
{

    FILE *src_file = fopen(src_path, "r");
    if (!src_file)
    {
        perror("Error opening source");
        return false;
    }

    FILE *dst_file = fopen(dst_path, "w");
    if (!dst_file)
    {
        perror("Error creating dest");
        fclose(src_file);
        return false;
    }

    char buffer[1024];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src_file)) > 0)
    {
        fwrite(buffer, 1, bytes_read, dst_file);
    }

    fclose(src_file);
    fclose(dst_file);
    return true;
}

void install_template(const char *template_path)
{
    // Copy path and convert path to wsl
    char converted_path[512];
    snprintf(converted_path, sizeof(converted_path), "%s", template_path);
    convert_windows_path_to_wsl(converted_path);

    if (!ensure_template_dir_exists())
    {
        return;
    }

    if (!file_exists(converted_path))
    {
        printf("Error: The file %s does not exist\n", converted_path);
        return;
    }

    char *filename = get_filename(converted_path);

    // Build destination path inside .templates folder
    char destination_path[512];
    snprintf(destination_path, sizeof(destination_path), ".templates/%s", filename);
    free(filename);

    if (file_exists(destination_path))
    {
        printf("Template already exists: %s\n", destination_path);
        return;
    }
    if (copy_file(converted_path, destination_path))
        printf("Installed template: %s\n", destination_path);
}

const char *get_placeholder(
    const char *placeholder_name,
    const char *placeholder_keys[],
    const char *placeholder_values[],
    int num_placeholders)
{
    for (int i = 0; i < num_placeholders; i++)
        if (strcmp(placeholder_name, placeholder_keys[i]) == 0)
            return placeholder_values[i];
    return "";
}

void create_dir_from_line(const char *line, const char *projectName)
{
    // Skip leading slashes and spaces
    const char *dir_name = line;
    while (*dir_name == '/' || *dir_name == ' ')
        dir_name++;

    // Skip empty lines
    if (*dir_name == '\0')
        return;

    size_t dir_length = strlen(dir_name);

    // Not a directory, skip
    if (dir_name[dir_length - 1] != '/')
    {
        return;
    }

    char path[512];
    snprintf(path, sizeof(path), "%s/%.*s", projectName, (int)(dir_length - 1), dir_name); // remove trailing /

    mkdir(path, 0755);
}

static void trim_trailing_whitespace(char *string)
{
    size_t length = strlen(string);
    while (length > 0 && isspace((unsigned char)string[length - 1]))
    {
        string[--length] = '\0';
    }
}

static void ensure_parent_dirs(const char *file_path)
{
    char tmp[512];
    strcpy(tmp, file_path);
    char *dir = dirname(tmp);
    if (dir)
    {
        mkdir(dir, 0755);
    }
}

static void replace_placeholders(
    const char *input_line,
    const char **placeholder_keys,
    const char **replacement_values,
    int key_count,
    char *output,
    size_t output_size)
{
    output[0] = '\0';
    const char *cursor = input_line;

    while (*cursor)
    {
        const char *start_bracket = strchr(cursor, '[');

        if (!start_bracket)
        {
            strncat(output, cursor, output_size - strlen(output) - 1);
            break;
        }

        strncat(output, cursor, start_bracket - cursor);

        const char *end_bracket = strchr(start_bracket, ']');

        if (!end_bracket)
        {
            strncat(output, start_bracket, output_size - strlen(output) - 1);
            break;
        }

        char placeholder[128];
        size_t key_len = end_bracket - start_bracket - 1;
        if (key_len >= sizeof(placeholder))
        {
            key_len = sizeof(placeholder) - 1;
        }
        strncpy(placeholder, start_bracket + 1, key_len);
        placeholder[key_len] = '\0';

        const char *replacement = "";
        for (int i = 0; i < key_count; i++)
        {
            if (strcmp(placeholder, placeholder_keys[i]) == 0)
            {
                replacement = replacement_values[i];
                break;
            }
        }

        strncat(output, replacement, output_size - strlen(output) - 1);
        cursor = end_bracket + 1;
    }
}

void create_file_from_line(
    FILE *template_fp,
    char *output_path,
    const char **placeholder_keys,
    const char **replacement_values,
    int key_count)
{
    trim_trailing_whitespace(output_path);
    ensure_parent_dirs(output_path);

    FILE *output_fp = fopen(output_path, "w");

    if (!output_fp)
    {
        perror("File create failed");
        return;
    }

    char input_line[1024];
    char processed_line[4096];

    while (fgets(input_line, sizeof(input_line), template_fp))
    {
        if (strncmp(input_line, "// ", 3) == 0)
        {
            fseek(template_fp, -strlen(input_line), SEEK_CUR); // Backtrack for next file
            break;
        }

        replace_placeholders(
            input_line,
            placeholder_keys,
            replacement_values,
            key_count,
            processed_line,
            sizeof(processed_line));

        fputs(processed_line, output_fp);
    }

    fclose(output_fp);
}

void create_package_json_file(const char *project_name, const char *dependencies[], int num_deps)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/package.json", project_name);

    FILE *f = fopen(path, "w");
    if (!f)
    {
        perror("Failed to create package.json");
        return;
    }

    fprintf(f, "{\n");
    fprintf(f, "  \"name\": \"%s\",\n", project_name);
    fprintf(f, "  \"version\": \"1.0.0\",\n");
    fprintf(f, "  \"private\": true,\n");

    // scripts section
    fprintf(f, "  \"scripts\": {\n");
    fprintf(f, "    \"dev\": \"next dev\",\n");
    fprintf(f, "    \"build\": \"next build\",\n");
    fprintf(f, "    \"start\": \"next start\"\n");
    fprintf(f, "  },\n");

    // dependencies section
    fprintf(f, "  \"dependencies\": {\n");
    for (int i = 0; i < num_deps; i++)
    {
        fprintf(f, "    \"%s\": \"*\"%s\n", dependencies[i], i < num_deps - 1 ? "," : "");
    }
    fprintf(f, "  }\n");

    fprintf(f, "}\n");

    fclose(f);
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
                create_dir_from_line(line + 2, projectName);
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
            create_dir_from_line(line + 2, projectName);
        }
    }

    printf("Project %s%s%s created using the %s%s%s template\n",
           STYLE_GREEN, projectName, STYLE_RESET,
           STYLE_GREEN, templateName, STYLE_RESET);

    fclose(templateFile);
}