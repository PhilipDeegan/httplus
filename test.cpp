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

std::vector<std::string> httplus::http::Reponder::TXT = {"xml", "txt", "html", "css"};

int main(int argc, char* argv[]) {
    kul::Signal sig;
    httplus::App a;
    httplus::Sites sites;
    httplus::yaml::Conf::LOAD(sites);
    kul::hash::map::S2T<std::shared_ptr<httplus::http::Server>> http;
    kul::hash::map::S2T<std::shared_ptr<httplus::https::Server>> https;
    a.load(http, https, sites);
    std::vector<std::pair<std::shared_ptr<std::reference_wrapper<httplus::http::Server>>, std::shared_ptr<kul::Thread>>> thr;
    for(const auto& site : http){
        httplus::http::Server& s(*site.second.get());
        std::shared_ptr<std::reference_wrapper<httplus::http::Server>> ref = std::make_shared<std::reference_wrapper<httplus::http::Server>>(std::ref(s));
        std::shared_ptr<kul::Thread> th = std::make_shared<kul::Thread>(*ref.get());
        thr.push_back(std::make_pair(ref, th));
        auto st = [&s](int16_t){ s.stop(); };
        sig.intr(st).segv(st);
    }
    std::vector<std::pair<std::shared_ptr<std::reference_wrapper<httplus::https::Server>>, std::shared_ptr<kul::Thread>>> thrS;
    for(const auto& site : https){
        httplus::https::Server& s(*site.second.get());
        std::shared_ptr<std::reference_wrapper<httplus::https::Server>> ref = std::make_shared<std::reference_wrapper<httplus::https::Server>>(std::ref(s));
        std::shared_ptr<kul::Thread> th = std::make_shared<kul::Thread>(*ref.get());
        thrS.push_back(std::make_pair(ref, th));
        auto st = [&s](int16_t){ s.stop(); };
        sig.intr(st).segv(st);
    }
    try{
        for(auto& t : thr) t.second->run();
        for(auto& t : thr) if(t.second->exception()) std::rethrow_exception(t.second->exception());
        for(auto& t : thrS) t.second->run();
        for(auto& t : thrS) if(t.second->exception()) std::rethrow_exception(t.second->exception());
        while(1){
            kul::this_thread::sleep(1000);
            std::exception_ptr e;
            for(auto& t : thr)
            if(t.second->exception()) e = t.second->exception();
            if(e){
                for(const auto site : http)  site.second->stop();
                for(const auto site : https) site.second->stop();
                std::rethrow_exception(e);
            }
        }
    }
    catch(const kul::Exit& e){ if(e.code() != 0) KERR << e.stack(); return e.code(); }
    catch(const kul::proc::ExitException& e){ KERR << e.stack(); return 1;}
    catch(const kul::Exception& e){ KERR << e.stack(); return 2;}
    catch(const std::exception& e){ KERR << e.what(); return 3;}
    return 0;
}
