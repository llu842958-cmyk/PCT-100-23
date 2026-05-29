#include "key.h"

int key1_pin = 20;

int key2_pin = 21;

void key_init()
{
    pinMode(key1_pin, INPUT);

    pinMode(key2_pin, INPUT);
}