/**
Copyright (c) 2023, Philip Deegan.
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
#ifndef HTTPLUS_YAML_HPP_
#define HTTPLUS_YAML_HPP_

#include "mkn/kul/yaml.hpp"

#include <filesystem>
#include <string>

namespace httplus::yaml {

// httplus.yaml
class Conf : public mkn::kul::yaml::File {
 public:
  static Conf CREATE(std::filesystem::path const& dir = std::filesystem::current_path());

  // directory holding httplus.yaml, relative paths in the config are resolved against it
  std::filesystem::path const& dir() const { return _dir; }
  mkn::kul::yaml::Validator validator() const override;

 private:
  Conf(std::string const& file)
      : mkn::kul::yaml::File(file), _dir(std::filesystem::path(file).parent_path()) {}

  std::filesystem::path _dir;

  friend class mkn::kul::yaml::File;
};

}  // namespace httplus::yaml

#endif /* HTTPLUS_YAML_HPP_ */
