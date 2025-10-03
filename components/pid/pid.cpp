/*
 *
 *  Created on: Feb 8, 2018
 *      Author: dig
 */

#include "include/pid.h"
#include <stdlib.h>
#include <stdio.h>

Pid::Pid( int id) {
	_id = id;
}

void Pid::setDesiredValue( float value ){
	desired = value;
}

void Pid::setPIDValues ( float Pval,float Ival, float Dval){
	P = Pval;
	I= Ival;
	D= Dval;
}

void Pid::setImaxImin (float Imax, float Imin){
	_iMax = Imax;
	_iMin = Imin;
}
void Pid::clear() {
	iSum = 0;
}

float Pid::update ( float measuredValue ){

	if (firstSample) {
		firstSample = false;
		oldMeasuredValue = measuredValue;
	}
	
 	delta = desired - measuredValue;
	
	result = delta * P;
	if (delta > 0 )
		iSum += I;
	else
		iSum -= I;
	
	if (iSum > 0) {
		if (iSum > _iMax)  // limit to maxI
			iSum = _iMax;
	} else {
		if (iSum < _iMin) // or negative value
			iSum = _iMin;
	}
	if ( _id == 1)
		printf("\ndelta: %f P:%f I:%f ", delta, result, iSum);

	result += iSum;
	result += (measuredValue-oldMeasuredValue) * D;
	oldMeasuredValue = measuredValue;

	if ( _id == 1)
		printf( "PID %%: %1.1f\n\r",  result);
	
	return result;
}





