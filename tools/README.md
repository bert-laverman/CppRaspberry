<!--
  -- Copyright (c) 2026 by Bert Laverman. All Rights Reserved.
  --
  -- Licensed under the Apache License, Version 2.0 (the "License");
  -- you may not use this file except in compliance with the License.
  -- You may obtain a copy of the License at
  --
  --    http://www.apache.org/licenses/LICENSE-2.0
  --
  -- Unless required by applicable law or agreed to in writing, software
  -- distributed under the License is distributed on an "AS IS" BASIS,
  -- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  -- See the License for the specific language governing permissions and
  -- limitations under the License.
  -->

# Tools

* [`cppr-deploy`](cppr-deploy): build a CppRaspberry project on another machine (a Raspberry Pi 5, say) and deploy it: a
  `Pico*` project is flashed to a Pico plugged into that machine, a `Zero2W*` project is copied to a Zero. It expects the
  library and the projects side by side under `~/dev/CppRaspberry/`, synchronises them to the same place on the build
  host, and builds there. Run `cppr-deploy --help`, or `cppr-deploy --list-picos` to see which Picos are attached.
  Put a symlink in a directory in your `PATH`, for example
  `ln -s ~/dev/CppRaspberry/CppRaspberry/tools/cppr-deploy ~/bin/cppr-deploy`.

  The build host needs `cmake`, `ninja`, the cross compiler for the Zero (`g++-arm-linux-gnueabihf`) with a CMake
  toolchain file in `~/cmake/armhf-debian.cmake`, the Pico SDK in `~/pico-sdk`, and `picotool` in `~/bin`.
* [`i2c-bus-tests`](i2c-bus-tests): programs and a script to find out what happens on the I2C bus.
