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
#ifndef INCLUDED_QuadratureCounter_detail_h_GUID_C7780BF4_CC62_4890_657D_1E186589977C
#define INCLUDED_QuadratureCounter_detail_h_GUID_C7780BF4_CC62_4890_657D_1E186589977C


// Internal Includes
// - none

// Library/third-party includes
#include <loki/Typelist.h>

// Standard includes
// - none

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
} // end of namespace Quadrature
#endif // INCLUDED_QuadratureCounter_detail_h_GUID_C7780BF4_CC62_4890_657D_1E186589977C

