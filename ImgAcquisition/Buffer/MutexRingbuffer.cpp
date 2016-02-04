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

MutexRingbuffer::MutexRingbuffer(int pSize){

	_Frontp=0;
	_Backp=0;
	_Size=pSize;
	_Images = new FlyCapture2::Image*[_Size];
	//Initialize all image objects
	for(int i=0; i < _Size; i++)
		_Images[i] = new FlyCapture2::Image;
}

FlyCapture2::Image* MutexRingbuffer::front(){

	_Access.lock();
	FlyCapture2::Image* r = _Images[_Frontp];
	_Access.unlock();
	return r;
}

FlyCapture2::Image* MutexRingbuffer::back(){

	//Care when to call return - Only after unlocking.
	//So note the pointer to return
	FlyCapture2::Image* r;
	_Access.lock();

	//if _Frontp equals _Backp, then the cache is empyt!
	if(_Frontp != _Backp){

		//Move the back pointer and decrease count.
		r = _Images[_Backp];
		_Backp++;
		_Count--;

		//Wrap around
		if(_Backp == _Size-1) _Backp=0;
	}
	else
		r = NULL;

	_Access.unlock();
	return r;
}

void MutexRingbuffer::push(){

	_Access.lock();

	//Increase front pointer and count
	_Frontp++;
	_Count++;

	//Wrap around
	if(_Frontp == _Size-1)
		_Frontp=0;

	//This is bad...
	if(_Frontp == _Backp){
		std::cerr << "Buffer overrun detected. Exiting."<<std::endl;
		exit(0);
	}
	_Access.unlock();
}

} /* namespace beeCompress */
