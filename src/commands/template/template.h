#ifndef TEMPLATES_H
#define TEMPLATES_H

#include <stdbool.h>

void install_template(const char *templatePath);

bool file_exists(const char *path);

void convert_windows_path_to_wsl(char *path);

void generate_project_from_template(const char *templateName, const char *projectName);

#endif /* TEMPLATES_H */