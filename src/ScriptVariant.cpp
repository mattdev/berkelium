/*  Berkelium - Embedded Chromium
 *  ScriptVariant.cpp
 *
 *  Copyright (c) 2010, Patrick Reiter Horn
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "berkelium/ScriptVariant.hpp"

#include <cstdlib>

namespace Berkelium {
namespace Script {

	void Variant::initwc(const wchar_t* str, size_t length) {
		type = JSSTRING;
		if (str && length) {
			wchar_t* wide = new wchar_t[length + 1];
			memcpy(wide, str, (length + 1) * sizeof(wchar_t));
			strPointer = WideString::point_to(wide, length);
		} else {
			strPointer = WideString::empty();
		}
	}
	void Variant::initmb(const char* str, size_t length) {
		type = JSSTRING;
		if (str && length) {
			wchar_t* wide = new wchar_t[length + 1];
#ifdef _WIN32
			mbstowcs_s(&length, wide, length+1, str, length);
#else
			length = std::mbstowcs(wide, str, length);
#endif
			strPointer = WideString::point_to(wide, length);
		} else {
			strPointer = WideString::empty();
		}
	}
	void Variant::initdbl(double dblval) {
		type = JSDOUBLE;
		doubleValue = dblval;
	}
	void Variant::initbool(bool boolval) {
		type = JSBOOLEAN;
		doubleValue = boolval ? 0 : 1;
	}
	void Variant::initnull() {
		type = JSNULL;
	}
	void Variant::initvariant(const Variant& other) {
		switch (other.type) {
		case JSSTRING:
			initwc(other.strPointer.data(), other.strPointer.length());
			break;
		case JSDOUBLE:
			initdbl(other.doubleValue);
			break;
		case JSBOOLEAN:
			initbool(!!other.doubleValue);
			break;
		default:
			initnull();
			break;
		}
	}

	void Variant::destroy() {
		if (type == JSSTRING) {
			delete []strPointer.data();
		}
		initnull();
	}

	Variant::Variant(const Variant& other) {
		initvariant(other);
	}
	Variant& Variant::operator=(const Variant& other) {
		destroy();
		initvariant(other);
		return *this;
	}

	Variant::~Variant() {
		destroy();
	}

}
}
