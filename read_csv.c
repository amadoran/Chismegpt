#include <stdio.h>
#include <string.h>

void read_csv(char * buff, char *result[2])
{

    int column = 0;
    int row = 0;
    char *value = strtok(buff, ",");

    while (value)
    {
        if (column == 0)
        {
            result[0] = value;
        }

        if (column == 1)
        {
            result[1] = value;
        }
        value = strtok(NULL, ",");
        column++;
    }
}