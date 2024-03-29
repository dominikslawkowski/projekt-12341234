#include <stdio.h>
#include "string.h"

void str_trim_lf(char *arr, int length)
{
    int i;
    for (i = 0; i < length; i++)
    {
        if (arr[i] == '\n')
        {
            arr[i] = '\0';
            break;
        }
    }
}

void str_overwrite_stdout()
{
    fflush(stdout);
}

void str_overwrite_stdout_with_msg(char *msg)
{
    printf("\r%s", "> ");
    printf("\r> %s", msg);
    fflush(stdout);
}
