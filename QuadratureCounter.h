/** @file
	@brief Header

	@date 2012

	@author
	Ryan Pavlik
	<rpavlik@iastate.edu> and <abiryan@ryand.net>
	http://academic.cleardefinition.com/
	Iowa State University Virtual Reality Applications Center
	Human-Computer Interaction Graduate Program

*/

//           Copyright Iowa State University 2012.
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#ifndef INCLUDED_QuadratureCounter_h_GUID_0431F524_612E_4C8C_712F_307751FAC308
#define INCLUDED_QuadratureCounter_h_GUID_0431F524_612E_4C8C_712F_307751FAC308


// Internal Includes
#include <QuadratureCounter_detail.h>

// Library/third-party includes
#include <loki/ForEachType.h>
#include <PinChangeInt.h>

// Standard includes
#include <util/atomic.h>
#include <Arduino.h>


#ifdef ARDUSIM
#include <cstdio>
#define  SIMDEBUGMSG(X) std::fprintf(stderr, X "\n");
#else
#define SIMDEBUGMSG(X)
#endif



namespace Quadrature {
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
				InterruptPolicy::registerInterruptHandlers(&type::template isr<detail::PinA>, &type::template isr<detail::PinB>);
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

			template<typename Pin>
			static void isr() {
				self->handlers[Pin::index]();
			}

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


#endif // INCLUDED_QuadratureCounter_h_GUID_0431F524_612E_4C8C_712F_307751FAC308

