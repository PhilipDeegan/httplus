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
#ifndef _HTTPLUS_YAML_HPP_
#define _HTTPLUS_YAML_HPP_

#include "kul/yaml.hpp"

#include "httplus/html.hpp"

namespace httplus{ namespace yaml{

class Exception : public kul::Exception{
    public:
        Exception(const char*f, const uint16_t& l, const std::string& s) : kul::Exception(f, l, s){}
};

class Conf : public kul::yaml::File {
    private:
        const kul::Dir d;
    protected:      
        Conf(const kul::Dir d) : kul::yaml::File(d.join("httplus.yaml")), d(d){}
        static Conf CREATE(const kul::Dir& d){
            kul::File f("httplus.yaml", d);
            if(!f.is()) KEXCEPTION("httplus,yaml does not exist:\n" + f.full());
            return kul::yaml::File::CREATE<Conf>(d.path());
        }
    public: 
        Conf(const Conf& p) : kul::yaml::File(p), d(p.d){}
        const kul::Dir&   dir() const { return d; }
        const kul::yaml::Validator validator() const{
            using namespace kul::yaml;

            NodeValidator http("http", {
                NodeValidator("root", 1),
                NodeValidator("text"),
                NodeValidator("host"),
                NodeValidator("port"),
                NodeValidator("home")
            }, 0, NodeType::LIST);
            NodeValidator https("https", {
                NodeValidator("root", 1),
                NodeValidator("text"),                
                NodeValidator("host", 1),
                NodeValidator("port"),
                NodeValidator("crt", 1),
                NodeValidator("key", 1),
                NodeValidator("chain"),
                NodeValidator("ssls"),
                NodeValidator("home")
            }, 0, NodeType::LIST);
            return Validator({
                NodeValidator("ssls"),
                http, 
                https
            });
        }
        static Conf CREATE(){
            return Conf::CREATE(kul::Dir(kul::env::CWD()));
        }
        friend class kul::yaml::File;
};


}}
#endif /* _HTTPLUS_YAML_HPP_ */
