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
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*/

#ifndef NFC_NFCTECH_H
#define NFC_NFCTECH_H

#include <cmath>
#include <memory>
#include <algorithm>

#include <sdr/SignalType.h>
#include <sdr/SignalBuffer.h>
#include <sdr/RecordDevice.h>

#include <nfc/Nfc.h>

#define DEBUG_CHANNELS 10
#define DEBUG_SIGNAL_VALUE_CHANNEL 0
#define DEBUG_SIGNAL_FILTERED_CHANNEL 1
#define DEBUG_SIGNAL_VARIANCE_CHANNEL 2
#define DEBUG_SIGNAL_AVERAGE_CHANNEL 3
#define DEBUG_SIGNAL_DECODER_CHANNEL 4

namespace nfc {

// Buffer length for signal integration, must be power of 2^n
#define BUFFER_SIZE 1024

/*
 * Signal debugger
 */
struct SignalDebug
{
   unsigned int channels;
   unsigned int clock;

   sdr::RecordDevice *recorder;
   sdr::SignalBuffer buffer;

   float values[10] {0,};

   SignalDebug(unsigned int channels, unsigned int sampleRate) : channels(channels), clock(0)
   {
      char file[128];
      struct tm timeinfo {};

      std::time_t rawTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

#ifdef _WIN32
      localtime_s(&timeinfo, &rawTime);
#else
      localtime_r(&rawTime, &timeinfo);
#endif

      strftime(file, sizeof(file), "decoder-%Y%m%d%H%M%S.wav", &timeinfo);

      recorder = new sdr::RecordDevice(file);
      recorder->setChannelCount(channels);
      recorder->setSampleRate(sampleRate);
      recorder->open(sdr::RecordDevice::Write);
   }

   ~SignalDebug()
   {
      delete recorder;
   }

   inline void block(unsigned int time)
   {
      if (clock != time)
      {
         // store sample buffer
         buffer.put(values, recorder->channelCount());

         // clear sample buffer
         for (auto &f: values)
         {
            f = 0;
         }

         clock = time;
      }
   }

   inline void set(int channel, float value)
   {
      if (channel >= 0 && channel < recorder->channelCount())
      {
         values[channel] = value;
      }
   }

   inline void begin(int sampleCount)
   {
      buffer = sdr::SignalBuffer(sampleCount * recorder->channelCount(), recorder->channelCount(), recorder->sampleRate(), 0, 0, sdr::SignalType::SAMPLE_REAL);
   }

   inline void write()
   {
      buffer.flip();
      recorder->write(buffer);
   }

   inline void close()
   {
      recorder->close();
   }
};

/*
 * pulse slot parameters (for pulse position modulation NFC-V)
 */
struct PulseSlot
{
   int start;
   int end;
   int value;
};

/*
 * baseband processor signal parameters
 */
struct SignalParams
{
   // factors for IIR DC removal filter
   float signalIIRdcA;

   // factors for exponential signal power
   float signalEnveW0;
   float signalEnveW1;

   // factors for exponential signal envelope
   float signalMeanW0;
   float signalMeanW1;

   // factors for exponential signal variance
   float signalMdevW0;
   float signalMdevW1;

   // 1/FC
   double sampleTimeUnit;

   // maximum silence
   int elementaryTimeUnit;
};

/*
 * bitrate timing parameters (one for each symbol rate)
 */
struct BitrateParams
{
   int rateType;
   int techType;

   // symbol parameters
   unsigned int symbolsPerSecond;
   unsigned int period0SymbolSamples;
   unsigned int period1SymbolSamples;
   unsigned int period2SymbolSamples;
   unsigned int period4SymbolSamples;
   unsigned int period8SymbolSamples;

   // modulation parameters
   unsigned int symbolDelayDetect;
   unsigned int offsetFutureIndex;
   unsigned int offsetSignalIndex;
   unsigned int offsetDelay0Index;
   unsigned int offsetDelay1Index;
   unsigned int offsetDelay2Index;
   unsigned int offsetDelay4Index;
   unsigned int offsetDelay8Index;

   // protocol specific parameters
   unsigned int preamble1Samples;
   unsigned int preamble2Samples;
};

/*
 * pulse position modulation parameters (for NFC-V)
 */
struct PulseParams
{
   int bits;
   int length;
   int periods;
   PulseSlot slots[256];
};

/*
 * sample parameters
 */
struct TimeSample
{
   float samplingValue; // sample raw value
   float filteredValue; // IIR-DC filtered value
   float meanDeviation; // standard deviation at sample time
   float modulateDepth; // modulation deep at sample time
};

/*
 * modulation status (one for each symbol rate)
 */
struct ModulationStatus
{
   // symbol search status
   unsigned int searchModeState;    // search mode / state control
   unsigned int searchStartTime;    // sample start of symbol search window
   unsigned int searchEndTime;      // sample end of symbol search window
   unsigned int searchSyncTime;     // sample at next synchronization
   unsigned int searchPulseWidth;   // detected signal pulse width
   float searchValueThreshold;      // signal value threshold
   float searchPhaseThreshold;      // signal phase threshold
   float searchLastPhase;           // auxiliary signal for last symbol phase
   float searchLastValue;           // auxiliary signal for value symbol
   float searchSyncValue;           // auxiliary signal value at synchronization point
   float searchCorrDValue;          // auxiliary value for last correlation difference
   float searchCorr0Value;          // auxiliary value for last correlation 0
   float searchCorr1Value;          // auxiliary value for last correlation 1

   // symbol parameters
   unsigned int symbolStartTime;    // sample time for symbol start
   unsigned int symbolEndTime;      // sample time for symbol end
   unsigned int symbolRiseTime;     // sample time for last rise edge

   // integrator processor
   float filterIntegrate;
   float detectIntegrate;
   float phaseIntegrate;

   // auxiliary detector peak values
   float correlatedPeakValue;
   float detectorPeakValue;

   // auxiliary detector peak times
   unsigned int correlatedPeakTime;     // sample time for maximum correlation peak
   unsigned int detectorPeakTime;     // sample time for maximum detector peak

   // data buffers
   float integrationData[BUFFER_SIZE];
   float correlationData[BUFFER_SIZE];
};

/*
 * status for one demodulated symbol
 */
struct SymbolStatus
{
   unsigned int pattern; // symbol pattern
   unsigned int value; // symbol value (0 / 1)
   unsigned long start;  // sample clocks for start
   unsigned long end; // sample clocks for end
   unsigned long edge; // sample clocks for last rise edge
   unsigned int length; // length of samples for symbol
   unsigned int rate; // symbol rate
};

/*
 * status for demodulate bit stream
 */
struct StreamStatus
{
   unsigned int previous;
   unsigned int pattern;
   unsigned int bits;
   unsigned int data;
   unsigned int flags;
   unsigned int parity;
   unsigned int bytes;
   unsigned char buffer[512];
};

/*
 * status for current frame
 */
struct FrameStatus
{
   unsigned int lastCommand; // last received command
   unsigned int frameType; // frame type
   unsigned int symbolRate; // frame bit rate
   unsigned int frameStart;  // sample clocks for start of last decoded symbol
   unsigned int frameEnd; // sample clocks for end of last decoded symbol
   unsigned int guardEnd; // frame guard end time
   unsigned int waitingEnd; // frame waiting end time

   // The frame delay time FDT is defined as the time between two frames transmitted in opposite directions
   unsigned int frameGuardTime;

   // The FWT defines the maximum time for a PICC to start its response after the end of a PCD frame.
   unsigned int frameWaitingTime;

   // The SFGT defines a specific guard time needed by the PICC before it is ready to receive the next frame after it has sent the ATS
   unsigned int startUpGuardTime;

   // The Request Guard Time is defined as the minimum time between the start bits of two consecutive REQA commands. It has the value 7000 / fc.
   unsigned int requestGuardTime;
};

struct DecoderStatus
{
   // signal parameters
   SignalParams signalParams {0,};

   // detected signal bitrate
   BitrateParams *bitrate = nullptr;

   // detected pulse code (for NFC-V)
   PulseParams *pulse = nullptr;

   // detected modulation
   ModulationStatus *modulation = nullptr;

   // signal data samples
   TimeSample sample[BUFFER_SIZE];

   // signal sample rate
   unsigned int sampleRate = 0;

   // signal master clock
   unsigned int signalClock = 0;

   // reference time for all decoded frames
   unsigned int streamTime = 0;

   // signal pulse filter
   unsigned int pulseFilter = 0;

   // minimum signal level
   float powerLevelThreshold = 0.01f;

   // signal raw value
   float signalValue = 0;

   // signal DC removed value (IIR filter)
   float signalFiltered = 0;

   // signal envelope value
   float signalEnvelope = 0;

   // signal average value
   float signalAverage = 0;

   // signal variance value
   float signalDeviation = 0;

   // signal DC-removal IIR filter (n sample)
   float signalFilterN0 = 0;

   // signal DC-removal IIR filter (n-1 sample)
   float signalFilterN1 = 0;

   // signal low threshold
   float signalLowThreshold = 0.0090f;

   // signal high threshold
   float signalHighThreshold = 0.0110f;

   // carrier trigger peak value
   float carrierEdgePeak = 0;

   // carrier trigger peak time
   unsigned int carrierEdgeTime = 0;

   // silence start (no modulation detected)
   unsigned int carrierOffTime = 0;

   // silence end (modulation detected)
   unsigned int carrierOnTime = 0;

   // signal debugger
   std::shared_ptr<SignalDebug> debug;

   // process next sample from signal buffer
   inline bool nextSample(sdr::SignalBuffer &buffer)
   {
      if (buffer.available() == 0 || buffer.type() != sdr::SignalType::SAMPLE_REAL)
         return false;

      // update signal clock and pulse filter
      ++signalClock;
      ++pulseFilter;

      buffer.get(signalValue);

      float signalDiff = std::abs(signalValue - signalEnvelope) / signalEnvelope;

      // signal average envelope detector
      if (signalDiff < 0.05f || pulseFilter > signalParams.elementaryTimeUnit * 10)
      {
         // reset silence counter
         pulseFilter = 0;

         // compute signal average
         signalEnvelope = signalEnvelope * signalParams.signalEnveW0 + signalValue * signalParams.signalEnveW1;
      }
      else if (signalClock < signalParams.elementaryTimeUnit)
      {
         signalEnvelope = signalValue;
      }

      // process new IIR filter value
      signalFilterN0 = signalValue + signalFilterN1 * signalParams.signalIIRdcA;

      // update signal value for IIR removal filter
      signalFiltered = signalFilterN0 - signalFilterN1;

      // update IIR filter component
      signalFilterN1 = signalFilterN0;

      // compute signal variance
      signalDeviation = signalDeviation * signalParams.signalMdevW0 + std::abs(signalFiltered) * signalParams.signalMdevW1;

      // process new signal envelope value
      signalAverage = signalAverage * signalParams.signalMeanW0 + signalValue * signalParams.signalMeanW1;

      // store signal components in process buffer
      sample[signalClock & (BUFFER_SIZE - 1)].samplingValue = signalValue;
      sample[signalClock & (BUFFER_SIZE - 1)].filteredValue = signalFiltered;
      sample[signalClock & (BUFFER_SIZE - 1)].meanDeviation = signalDeviation;
      sample[signalClock & (BUFFER_SIZE - 1)].modulateDepth = (signalEnvelope - std::clamp(signalValue, 0.0f, signalEnvelope)) / signalEnvelope;

      // get absolute DC-removed signal for edge detector
      float filteredRectified = std::fabs(signalFiltered);

      // detect last carrier edge on/off
      if (filteredRectified > signalHighThreshold)
      {
         // search maximum pulse value
         if (filteredRectified > carrierEdgePeak)
         {
            carrierEdgePeak = filteredRectified;
            carrierEdgeTime = signalClock;
         }
      }
      else if (filteredRectified < signalLowThreshold)
      {
         carrierEdgePeak = 0;
      }

      if (debug)
      {
         debug->block(signalClock);

         debug->set(DEBUG_SIGNAL_VALUE_CHANNEL, sample[signalClock & (BUFFER_SIZE - 1)].samplingValue);
         debug->set(DEBUG_SIGNAL_FILTERED_CHANNEL, sample[signalClock & (BUFFER_SIZE - 1)].filteredValue);
         debug->set(DEBUG_SIGNAL_VARIANCE_CHANNEL, sample[signalClock & (BUFFER_SIZE - 1)].meanDeviation);
         debug->set(DEBUG_SIGNAL_AVERAGE_CHANNEL, signalAverage);
      }

      return true;
   }
};

struct NfcTech
{
   unsigned short crc16(NfcFrame &frame, int from, int to, unsigned short init, bool refin);
};

}

#endif
