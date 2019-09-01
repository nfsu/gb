#pragma once
#include "types/types.hpp"

namespace gb {

	struct PSR {

		static constexpr u8 zMask = 0x80, nMask = 0x40, hMask = 0x20, cMask = 0x10;

		u8 v;

		//H: the last operation caused overflow into the higher nibble
		__forceinline bool carryHalf() const { return v & hMask; }

		//C: the last operation caused overflow into the 9th bit
		__forceinline bool carry() const { return v & cMask; }

		//Z: the last operation was zero
		__forceinline bool zero() const { return v & zMask; }

		//N: subtraction was performed last operation
		__forceinline bool subtract() const { return v & nMask; }

		//Setters

		__forceinline void carryHalf(bool b) { b ? v |= hMask : v &= ~hMask; }
		__forceinline void carry(bool b) { b ? v |= cMask : v &= ~cMask; }
		__forceinline void zero(bool b) { b ? v |= zMask : v &= ~zMask; }
		__forceinline void subtract(bool b) { b ? v |= nMask : v &= ~nMask; }

		__forceinline void setSubtract() { v |= nMask; }
		__forceinline void clearSubtract() { v &= ~nMask; }

		__forceinline void setHalf() { v |= hMask; }
		__forceinline void clearHalf() { v &= ~hMask; }

		__forceinline void setCarry() { v |= cMask; }
		__forceinline void clearCarry() { v &= ~cMask; }

		__forceinline void setZero() { v |= zMask; }
		__forceinline void clearZero() { v &= ~zMask; }

		template<typename T>
		__forceinline void setCodes(T a) {
			zero(a == 0);
		}

		template<bool sub, typename T>
		__forceinline void setALU(T a, T, T c) {

			carryHalf(c & 0x10);

			if constexpr (sub) {
				carry(c > a);
				setSubtract();
			} else {
				carry(c < a);
				clearSubtract();
			}

			setCodes(c);
		}

	};

}