/*
* Poor man's emulation of the C64 SID's envelope generator.
*
* WebSid (c) 2019 Jürgen Wothke
* version 0.93
* 
* Terms of Use: This software is licensed under a CC BY-NC-SA 
* (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/
#ifndef WEBSID_ENVELOPE_H
#define WEBSID_ENVELOPE_H

extern "C" {
#include "base.h"
}

/**
* This class handles the envelope generation for one specific voice of the SID chip.
*
* It is a construct exclusively used by the SID class and access is restricted accordingly.
*/
class Envelope {
protected:
	friend class SID;
	Envelope(class SID* sid, uint8_t voice);
		
	/**
	* Reinitialize a specific instance to reuse it.
	*/
	void reset();

	void clockEnvelope();	// +1 cycle
	
	/**
	* Handle those SID writes that impact the envelope generator.
	*/
	void poke(uint8_t reg, uint8_t val);
	
	/**
	* Gets the currently valid output envelope level.
	*/
	uint8_t getOutput();

	/**
	* Gets the raw AD register.
	*/
	uint8_t getAD();
	/**
	* Gets the raw SR register.
	*/
	uint8_t getSR();
private:
	void syncADR();
	uint8_t triggerLFSR_Threshold(uint16_t threshold, uint16_t* end);
	uint8_t handleExponentialDelay(struct EnvelopeState* state);
	
private:
	friend struct EnvelopeState* getState(Envelope *e);

	void* _state;	// don't want this header file cluttered with all the implementation details..
	class SID* _sid;
	uint8_t _voice;
};

#endif
