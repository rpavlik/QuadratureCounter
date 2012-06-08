#include <arduino.h>

/*
  Turns on an LED on for one second, then off for one second, repeatedly.

  This example code is in the public domain.
*/

#include <loki/Typelist.h>
#include <loki/TypelistMacros.h>
#include <loki/ForEachType.h>

#ifdef ARDUSIM
#include <cstdio>
#define  SIMDEBUGMSG(X) std::fprintf(stderr, X "\n");
#else
#define SIMDEBUGMSG(X)
#endif

template<bool A, bool B>
struct BoolPair {
	typedef BoolPair<A, B> type;
	enum {
		valueA = A,
		valueB = B
	};
};

typedef typename Loki::TL::MakeTypelist<BoolPair<0, 0>, BoolPair<0, 1>, BoolPair<1, 1>, BoolPair<1, 0> >::Result QuadratureSequence;

typedef void (*QuadratureInterruptHandler)(void);

int16_t counter = 0;
QuadratureInterruptHandler handlers[2];
/*

template<typename Current>
struct GetPrev {
	enum {
		value = (IndexOf<QuadratureSequence, Current>::value - 1 ) % 4
	};

	typedef typename TypeAt<QuadratureSequence, value>::type type;
};
template<typename Current>
struct GetNext {
	enum {
		value = (IndexOf<QuadratureSequence, Current>::value + 1 ) % 4
	};
	typedef TypeAt<QuadratureSequence, value>::type type;
	enum {
		valueA = type::valueA,
		valueB = type::valueB
	};
};
*/

template<typename Current, int Direction>
struct Iterate {
	enum {
		value = (::Loki::TL::IndexOf<QuadratureSequence, Current>::value + Direction ) % 4
	};
	typedef typename ::Loki::TL::TypeAt<QuadratureSequence, value>::Result type;
	enum {
		valueA = type::valueA,
		valueB = type::valueB
	};
};
struct PinA {
	enum {
		index = 0
	};
};
struct PinB {
	enum {
		index = 1
	};
};

template<bool CurrentVal, bool NextVal>
struct ForwardIfDifferent {
	enum {
		value = 1
	};
};

template<bool Val>
struct ForwardIfDifferent<Val, Val> {
	enum {
		value = 0
	};
};

template<bool Forward>
struct GetDirectionValue;
template<>
struct GetDirectionValue<true> {
	enum {
		value = 1
	};
};

template<>
struct GetDirectionValue<false> {
	enum {
		value = -1
	};
};
template<typename Current, typename Pin>
struct GetDirectionForChanges;
template<typename Current>
struct GetDirectionForChanges <Current, PinA> {
	enum {
		value = ForwardIfDifferent<Iterate<Current, 1>::valueA, Current::valueA>::value
	};
};

template<typename Current>
struct GetDirectionForChanges <Current, PinB> {
	enum {
		value = ForwardIfDifferent<Iterate<Current, 1>::valueB, Current::valueB>::value
	};
};




template<typename NewState>
void setQuadratureState();

template<typename Current, typename Pin>
void handlePinChange() {
	enum {
		direction = GetDirectionForChanges<Current, Pin>::value
	};
	counter += direction;
	setQuadratureState<typename Iterate<Current, direction>::type> ();
}

template<typename NewState>
void setQuadratureState() {
	handlers[PinA::index] = &handlePinChange<NewState, PinA>;
	handlers[PinB::index] = &handlePinChange<NewState, PinB>;
}

namespace {
	struct Callable {

		bool valA;
		bool valB;
		template<int Index, typename type>
		inline void operator()() {
			//typedef typename ::Loki::TL::TypeAt<QuadratureSequence, Index>::Result type;
			if (valA == type::valueA && valB == type::valueB) {
				SIMDEBUGMSG("got it!");
				setQuadratureState<type>();
			} else {

				SIMDEBUGMSG("miss!");
			}
		}
	};
	inline void setQuadratureState(bool A, bool B) {
		SIMDEBUGMSG("starting foreach");
		Callable c = {A, B};
		Loki::ForEachType<QuadratureSequence, Callable> dummy(c);
	}
}

void setup()
{
	// initialize the digital pin as an output.
	// Pin 13 has an LED connected on most Arduino boards:
	pinMode(13, OUTPUT);

	Serial.begin(9600);
	pinMode(4, INPUT);
	pinMode(5, INPUT);
	setQuadratureState(digitalRead(4), digitalRead(5));
}

void loop()
{
	digitalWrite(13, HIGH);   // set the LED on
	delay(1000);              // wait for a second
	digitalWrite(13, LOW);    // set the LED off
	delay(1000);              // wait for a second
}
