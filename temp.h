#ifndef __TEMP_H__
#define __TEMP_H__

extern float fan_on_temp;
extern float fan_off_temp;
extern float current_temp;

void temp_init();
void temp_update();
void temp_reset();

#endif
