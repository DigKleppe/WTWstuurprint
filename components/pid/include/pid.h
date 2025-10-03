/*
 * pid.h
 *
 *
 *  Created on:  july 28, 2025
 *      Author: dig
 */

#ifndef MAIN_INCLUDE_PID_H_
#define MAIN_INCLUDE_PID_H_

#include <stdint.h>

class Pid {
public:
  Pid(int id = 0);
  void setDesiredValue( float value );
  void setPIDValues ( float Pval,float Ival, float Dval);
  void setImaxImin (float Imax, float Imin);
  void clear(void);
  float update ( float measuredValue );

 
private:
  int _id;
  float _iMax = 100;
  float _iMin = 0;
  float P = 0,I = 0, D = 0;
  float desired = 0;
  float actual = 0;
  float delta;
 	float result;
 	float iSum ;
	float oldMeasuredValue;
  bool firstSample = true;

};

#endif
