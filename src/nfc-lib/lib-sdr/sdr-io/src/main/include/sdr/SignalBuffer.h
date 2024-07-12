/*

  Copyright (c) 2021 Jose Vicente Campos Martinez - <josevcm@gmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*/

#ifndef SDR_SIGNALBUFFER_H
#define SDR_SIGNALBUFFER_H

#include <memory>

#include <rt/Buffer.h>

namespace sdr {

class SignalBuffer : public rt::Buffer<float>
{
      struct Impl;

   public:

      SignalBuffer();

      SignalBuffer(unsigned int length, unsigned int stride, unsigned int samplerate, unsigned int offset, unsigned int decimation, int type, void *context = nullptr);

      SignalBuffer(float *data, unsigned int length, unsigned int stride, unsigned int samplerate, unsigned int offset, unsigned int decimation, int type, void *context = nullptr);

      SignalBuffer(const SignalBuffer &other);

      SignalBuffer &operator=(const SignalBuffer &other);

      unsigned int offset() const;

      unsigned int decimation() const;

      unsigned int sampleRate() const;

   private:

      std::shared_ptr<Impl> impl;
};

}

#endif //NFC_LAB_SIGNALBUFFER_H
