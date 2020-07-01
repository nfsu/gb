#pragma once
#include "addresses.hpp"

namespace gb {

	struct PSR {

		static constexpr u8
			zMask = 0x80,
			sMask = 0x40,
			hMask = 0x20,
			cMask = 0x10;

		u8 v;

		//H: the last operation caused overflow into the higher nibble
		_inline_ bool carryHalf() const { return v & hMask; }

		//C: the last operation caused overflow into the 9th bit
		_inline_ bool carry() const { return v & cMask; }

		//Z: the last operation was zero
		_inline_ bool zero() const { return v & zMask; }

		//N: subtraction was performed last operation
		_inline_ bool subtract() const { return v & sMask; }

		//Setters

		_inline_ void carryHalf(bool b) { clearHalf(); v |= b * hMask; }
		_inline_ void carry(bool b) { clearCarry(); v |= b * cMask; }
		_inline_ void zero(bool b) { clearZero(); v |= b * zMask; }
		_inline_ void subtract(bool b) { clearSubtract(); v |= b * sMask; }

		_inline_ void setSubtract() { v |= sMask; }
		_inline_ void clearSubtract() { v &= ~sMask; }

		_inline_ void setHalf() { v |= hMask; }
		_inline_ void clearHalf() { v &= ~hMask; }

		_inline_ void setCarry() { v |= cMask; }
		_inline_ void clearCarry() { v &= ~cMask; }

		_inline_ void setZero() { v |= zMask; }
		_inline_ void clearZero() { v &= ~zMask; }

		template<typename T>
		__forceinline void setCodes(T a) {
			zero(a == 0);
		}

		template<bool sub, typename T>
		_inline_ void setALU(T a, T, T c) {

			carryHalf(c & 0x10);

			if constexpr (sub) {
				carry(c > a);
				setSubtract();
			} else {
				carry(c < a);
				clearSubtract();
			}

			zero(a == 0);
		}

	};

}