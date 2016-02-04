/*
 * MutexLinkedList.cpp
 *
 *  Created on: Jan 25, 2016
 *      Author: hauke
 */

#include "MutexLinkedList.h"
#include <iostream>

namespace beeCompress {

MutexLinkedList::MutexLinkedList() {
	// Nothing to do here: Auto-generated constructor stub
}

MutexLinkedList::~MutexLinkedList() {
	// Nothing to do here: Auto-generated destructor stub
}


void MutexLinkedList::push(ImageBuffer imbuffer){
	_Access.lock();
	images.push_back(imbuffer);
	_Access.unlock();
}

ImageBuffer MutexLinkedList::pop(){
	_Access.lock();
	if(images.size()>0){
		ImageBuffer ret(images.front());
		images.pop_front();
		_Access.unlock();
		return ret;
	}
	_Access.unlock();
	ImageBuffer dummy(0,0,0,"");
	return dummy;
}

} /* namespace beeCompress */
