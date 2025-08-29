#ifndef TSENSORTASK_H_
#define TSENSORTASK_H_

#define ERRORTEMP 9999.0f

extern int binnenTemperatuur, buitenTemperatuur;
void temperatureSensorTask(void *pvParameter);
#endif