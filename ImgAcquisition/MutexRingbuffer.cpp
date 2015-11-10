/*
 * MutexRingbuffer.cpp
 *
 *  Created on: Nov 10, 2015
 *      Author: hauke
 */

#include "MutexRingbuffer.h"
#include <iostream>

namespace beeCompress {

MutexRingbuffer::~MutexRingbuffer() {
	// TODO Auto-generated destructor stub
}

MutexRingbuffer::MutexRingbuffer(){
	int pSize = 500;
	_Frontp=0;
	_Backp=0;
	_Size=pSize;
	_Images = new FlyCapture2::Image*[_Size];
	for(int i=0; i<_Size; i++)
		_Images[i] = new FlyCapture2::Image;
}

FlyCapture2::Image* MutexRingbuffer::front(){
	_Access.lock();
	FlyCapture2::Image* r = _Images[_Frontp];
	_Access.unlock();
	return r;
}

FlyCapture2::Image* MutexRingbuffer::back(){
	FlyCapture2::Image* r;
	_Access.lock();
	if(_Frontp!=_Backp){
		r = _Images[_Backp];
		_Backp++;
		_Count--;
		if(_Backp==_Size) _Frontp=0;
	}
	else
		r = NULL;
	_Access.unlock();
	return r;
}

void MutexRingbuffer::push(){
	_Access.lock();
	_Frontp++;
	_Count++;
	if(_Frontp==_Size)
		_Frontp=0;
	if(_Frontp==_Backp){
		std::cerr << "Buffer overrun detected. Exiting."<<std::endl;
		exit(0);
	}
	_Access.unlock();
}

} /* namespace beeCompress */
