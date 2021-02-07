#ifndef __SND_INTERP_H__
#define __SND_INTERP_H__

#define t_double float

// simple interface that could easily be recycled

class foo_interpolate
{
public:
	foo_interpolate() {}
	virtual ~foo_interpolate() {};

	virtual void reset() = 0;

	virtual void push(int sample) = 0;
	virtual int pop(t_double rate) = 0;
};

foo_interpolate * get_filter(int which);


// complicated, synced interface, specific to this implementation

t_double calc_rate(int timer);

extern "C" {
void interp_setup(int which);
void interp_cleanup();
}

void interp_switch(int which);

void interp_reset(int ch);
void interp_push(int ch, int sample);
int interp_pop(int ch, t_double rate);

#endif

