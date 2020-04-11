#ifndef MOISTURE_H
#define MOISTURE_H

#include "driver/adc_common.h"

int moisture_read(void);
void moisture_init(adc1_channel_t pin);

#endif /* MOISTURE_H */
