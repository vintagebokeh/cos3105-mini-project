#ifndef COMMON_H
#define COMMON_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/file.h>

const char *statusName(int status);
char* replace_char(char* str, char find, char replace);
int isNumeric(const char *str);
int isFloat(const char *str);
int isEmptyOrSpace(const char *str);
int fileExists(const char *filename);
#endif