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


void MutexLinkedList::push(std::shared_ptr<ImageBuffer> imbuffer){
	_Access.lock();
	//std::cout << "PUSH1"<<std::endl;
	images.push_back(std::move(imbuffer));

	//std::cout << "PUSH2"<<std::endl;
	_Access.unlock();
}

std::shared_ptr<beeCompress::ImageBuffer> MutexLinkedList::pop(){
	_Access.lock();
	if(images.size()>0){
		//std::cout << "POP1"<<std::endl;
		std::shared_ptr<ImageBuffer> img = std::move(images.front());
		//std::cout << "POP2"<<std::endl;
		images.pop_front();
		//std::cout << "POP3"<<std::endl;
		_Access.unlock();
		return img;
	}
	_Access.unlock();
	std::shared_ptr<ImageBuffer> dummy(new beeCompress::ImageBuffer(0,0,0,""));
	return dummy;
}

} /* namespace beeCompress */
