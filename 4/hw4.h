// header.h

#ifndef hw4_H
# define hw4_H

# include <stdio.h>

#define BUF_SIZE 100
#define WORD_CNT 15
#define MAX_CLNT 256

#define COLOR_RED "\033[38;2;255;0;0m"
#define COLOR_RESET "\033[0m"

typedef struct Search_Record
{
	char word[BUF_SIZE];
	int num_searched;
} Search_Record;


typedef struct Input
{
	char input_word[BUF_SIZE];
} Input;


void *handle_clnt(void *arg);
void error_handling(char *msg);
void parse_textfile(char file_path[], Search_Record *record);
int compare(const void *a, const void *b);
void set_terminal_mode(int enable);
void show_with_color(char word_to_color[], char input[]);

#endif