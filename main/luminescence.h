#ifndef LUMINESCENCE_H
#define LUMINESCENCE_H

#include "driver/adc.h"

int luminescence_read(void);
void luminescence_init(adc1_channel_t pin);

#endif /* LUMINESCENCE_H */
