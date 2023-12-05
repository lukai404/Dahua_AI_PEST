#include "yolo_object.h"
#include <stdio.h>

const char *g_yolo_object_list[4] =
{
    "person",//00
    "car",
    "motorcycle"
};

char g_modelfile[] = "./model/Volt_nnie.nnx";

void fillBuffer(char* buffer, int* len)
{
    int length = *len;
    length += sprintf(buffer + length, "{\"id\":0, \"name\":\"person\"},");
    length += sprintf(buffer + length, "{\"id\":1, \"name\":\"bicycle\"},");
    length += sprintf(buffer + length, "{\"id\":2, \"name\":\"motorcycle\"}");
    *len = length;
}


