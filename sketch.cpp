#include <arduino.h>

/*
  Experimenting with handling a quadrature encoder with template metaprogramming.
*/

#include <loki/Typelist.h>
#include <loki/TypelistMacros.h>
#include <loki/ForEachType.h>
#include <util/atomic.h>

#include <PinChangeInt.h>

#ifdef ARDUSIM
#include <cstdio>
#define  SIMDEBUGMSG(X) std::fprintf(stderr, X "\n");
#else
#define SIMDEBUGMSG(X)
#endif

namespace Quadrature {
	typedef void (*QuadratureInterruptHandler)(void);
	namespace detail {
		template<bool A, bool B>
		struct BoolPair {
			typedef BoolPair<A, B> type;
			enum {
				valueA = A,
				valueB = B
			};
		};

		typedef
			Loki::TL::MakeTypelist<BoolPair<0, 0>, BoolPair<0, 1>, BoolPair<1, 1>, BoolPair<1, 0> >::Result
			QuadratureSequence;

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

	} // end of namespace detail


	template <int PinNumA, int PinNumB>
	struct PinChangeInterruptLibPolicy {
		static void registerInterruptHandlers(QuadratureInterruptHandler handlerA, QuadratureInterruptHandler handlerB) {
			PCattachInterrupt(PinNumA, handlerA, CHANGE);
			PCattachInterrupt(PinNumB, handlerB, CHANGE);
		}
	};

	template<int Pin>
	struct ExternalInterruptNumberTrait;

	#define EXTERNAL_INTERRUPT_TRAIT(PIN, NUM) \
		template<> \
		struct ExternalInterruptNumberTrait<PIN> { \
			enum { \
				value = NUM \
			}; \
		}

	EXTERNAL_INTERRUPT_TRAIT(2, 0);
	EXTERNAL_INTERRUPT_TRAIT(3, 1);

	/// @todo add Mega support here: 2 (pin 21), 3 (pin 20), 4 (pin 19), and 5 (pin 18).
	#undef EXTERNAL_INTERRUPT_TRAIT

	template <int PinNumA, int PinNumB>
	struct ExternalInterruptPolicy {
		static void registerInterruptHandlers(QuadratureInterruptHandler handlerA, QuadratureInterruptHandler handlerB) {
			attachInterrupt(ExternalInterruptNumberTrait<PinNumA>::value, handlerA, CHANGE);
			attachInterrupt(ExternalInterruptNumberTrait<PinNumB>::value, handlerB, CHANGE);
		}
	};

	template <int PinNumA, int PinNumB>
	struct DefaultInterruptPolicy : PinChangeInterruptLibPolicy<PinNumA, PinNumB> {};

	// Hardware external interrupts
	template <>
	struct DefaultInterruptPolicy<2, 3> : ExternalInterruptPolicy<2, 3> {};

	template<int PinNumA, int PinNumB, typename CounterType = int16_t, typename InterruptPolicy = DefaultInterruptPolicy<PinNumA, PinNumB> >
	class QuadratureCounter {
		public:
			typedef QuadratureCounter<PinNumA, PinNumB, CounterType> type;
			typedef CounterType value_type;

			QuadratureCounter()
				: counter(0) {
				self = this;
			}

			void begin() {
				pinMode(PinNumA, INPUT);
				pinMode(PinNumB, INPUT);
				_setInitialQuadratureState(digitalRead(PinNumA), digitalRead(PinNumB));
				/// @todo register pinchangeint here
			}

			value_type getCounter() {
				value_type ctrcopy;
				ATOMIC_BLOCK(ATOMIC_RESTORESTATE )
				{
					ctrcopy = counter;
				}
				return ctrcopy;
			}
		private:
			static type * self;
			volatile CounterType counter;
			volatile QuadratureInterruptHandler handlers[2];


			template<typename Current, typename Pin>
			static void handlePinChange() {
				enum {
					direction = detail::GetDirectionForChanges<Current, Pin>::value
				};
				self->counter += direction;
				setQuadratureState<typename detail::Iterate<Current, direction>::type> ();
			}

			template<typename NewState>
			static void setQuadratureState() {
				self->handlers[detail::PinA::index] = &handlePinChange<NewState, detail::PinA>;
				self->handlers[detail::PinB::index] = &handlePinChange<NewState, detail::PinB>;
			}

			struct ConditionalQuadratureStateSetter {
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

			inline void _setInitialQuadratureState(bool A, bool B) {
				SIMDEBUGMSG("starting foreach");
				ConditionalQuadratureStateSetter c = {A, B};
				Loki::ForEachType<detail::QuadratureSequence, ConditionalQuadratureStateSetter> dummy(c);
			}
	};
	// Definition of static variable in template.
	template<int PinNumA, int PinNumB, typename CounterType, typename InterruptPolicy>
	typename QuadratureCounter<PinNumA, PinNumB, CounterType, InterruptPolicy>::type * QuadratureCounter<PinNumA, PinNumB, CounterType, InterruptPolicy>::self = NULL;

} // end of namespace Quadrature

Quadrature::QuadratureCounter<4, 5> qc;
void setup()
{
	Serial.begin(9600);
	qc.begin();
}

void loop()
{
	digitalWrite(13, HIGH);   // set the LED on
	delay(1000);              // wait for a second
	digitalWrite(13, LOW);    // set the LED off
	delay(1000);              // wait for a second
}
