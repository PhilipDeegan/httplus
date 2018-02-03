/**
Copyright (c) 2016, Philip Deegan.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution.
    * Neither the name of Philip Deegan nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef _HTTPLUS_PAGE_HPP_
#define _HTTPLUS_PAGE_HPP_

#include "kul/log.hpp"
#include "kul/http.hpp"
#include "kul/html4.hpp"
#include "kul/threads.hpp"

namespace httplus{

class Page : public kul::html4::Page{
	protected:
    	template <class T> std::shared_ptr<T> clone(const T& src){
			return std::make_shared<T>();
		}
    public:
    	virtual std::shared_ptr<Page> clone() = 0 ;//{ return clone(*this); }
        virtual void pre (const kul::http::A1_1Request& req){}
        virtual void post(const kul::http::A1_1Request& req, kul::http::Response& res){}
};
typedef kul::hash::map::S2T<std::shared_ptr<Page>> Pages;

class XXXError{
    public:
        virtual void recover(Page& e) = 0;
};

typedef kul::hash::map::S2T<std::shared_ptr<Page>> Pages;
typedef kul::hash::map::S2T<Pages> Sites;

}
#endif /* _HTTPLUS_PAGE_HPP_ */
