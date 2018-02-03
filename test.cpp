/**
Copyright (c) 2013, Philip Deegan.
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
#include "kul/signal.hpp"

#include "httplus.hpp"
#include "html.hpp"

int main(int argc, char* argv[]) {
    kul::Signal sig;
    httplus::App a;
    httplus::Sites sites;

    httplus::Pages glbP;
    glbP.insert("404",         std::make_shared<_404>());
    glbP.insert("index",       std::make_shared<Index>());
    glbP.insert("res/css.css", std::make_shared<CSS>());
    sites.insert(std::to_string(std::hash<std::string>()("/var/www/global")), glbP);

    kul::hash::map::S2T<std::shared_ptr<httplus::http::AServer>> servers;
    a.load(servers, sites);
    std::vector<std::pair<std::shared_ptr<httplus::http::AServer>, std::shared_ptr<kul::Thread>>> thr;
    for(const auto& site : servers){
        auto& s(site.second);
        std::shared_ptr<kul::Thread> th = std::make_shared<kul::Thread>(std::ref(*s.get()));
        thr.push_back(std::make_pair(s, th));
        auto st = [&s](int16_t){ s->stop(); };
        sig.intr(st).segv(st);
    }
    try{
        for(auto& t : thr) t.second->run();
        while(1){
            kul::this_thread::sleep(100);            
            std::exception_ptr e;
            for(auto& t : thr)
                if(t.second->exception()){
                    e = t.second->exception();
                    if(e) break;
                }
            if(e){
                for(const auto site : servers) site.second->stop();
                std::rethrow_exception(e);
            }
        }
    }
    catch(const kul::Exit& e){ if(e.code() != 0) KERR << e.stack(); return e.code(); }
    catch(const kul::Exception& e){ KERR << e.stack(); return 2;}
    catch(const std::exception& e){ KERR << e.what(); return 3;}
    catch(...)                    { KLOG(ERR) << "UNKNOWN EXCEPTION CAUGHT"; return 5;}
    return 0;
}
