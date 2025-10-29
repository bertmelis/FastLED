#pragma once

#ifndef __INC_CHIPSETS_H
#define __INC_CHIPSETS_H

#include "pixeltypes.h"
#include "fl/five_bit_hd_gamma.h"
#include "fl/force_inline.h"
#include "fl/bit_cast.h"
#include "pixel_iterator.h"
#include "crgb.h"
#include "eorder.h"
#include "fl/math_macros.h"
#include "fl/compiler_control.h"

// Include centralized LED chipset timing definitions
// These provide unified nanosecond-based T1, T2, T3 timing for all supported chipsets
#include "fl/chipsets/led_timing.h"

// Include legacy AVR-specific timing definitions (FMUL-based)
// Used for backward compatibility with existing AVR clockless drivers
#ifdef __AVR__
#include "platforms/avr/led_timing_legacy_avr.h"
#endif

// Platform-specific clockless controller
// This must be included before using ClocklessControllerImpl and before UCS7604
#if defined(__EMSCRIPTEN__)
  #include "platforms/wasm/clockless.h"
#elif defined(FASTLED_STUB_IMPL) && !defined(__EMSCRIPTEN__)
  #include "platforms/stub/clockless_stub_generic.h"
#elif defined(ESP32)
  #ifdef FASTLED_ESP32_I2S
    #include "platforms/esp/32/clockless_i2s_esp32.h"
  #else
    #include "third_party/espressif/led_strip/src/enabled.h"
    #if FASTLED_ESP32_HAS_RMT
      #include "platforms/esp/32/clockless_rmt_esp32.h"
    #elif FASTLED_ESP32_HAS_CLOCKLESS_SPI
      #include "platforms/esp/32/clockless_spi_esp32.h"
    #endif
  #endif
#elif defined(FASTLED_TEENSY4)
  #include "platforms/arm/teensy/teensy4_common/clockless_arm_mxrt1062.h"
#elif defined(__AVR__)
  #include "platforms/avr/clockless_trinket.h"
#endif

// Include generic ClocklessBlockController as fallback for platforms without specialized implementation
// This provides single-pin clockless LED support using nanosecond-precision timing
#include "platforms/shared/clockless_block/clockless_block_generic.h"

// Template alias to ClocklessController (platform-specific or generic blocking)
// This must come AFTER all clockless drivers are included
#include "fl/clockless_controller_impl.h"

// Include UCS7604 controller (requires ClocklessBlockController to be defined)
#include "fl/chipsets/ucs7604.h"  // optional.

// Include platform-independent SPI utilities
#include "platforms/shared/spi_pixel_writer.h"

// Include platform-specific SPIOutput template implementations
// Each platform file defines the SPIOutput template for its platform
// This must happen before any SPI chipset controllers (e.g., APA102, P9813)
#if defined(__EMSCRIPTEN__) || defined(FASTLED_STUB_IMPL)
#include "platforms/stub/spi_output_template.h"

#elif defined(ESP32) || defined(ESP32S2) || defined(ESP32S3) || defined(ESP32C3) || defined(ESP32P4)
#include "platforms/esp/32/spi_output_template.h"

#elif defined(ESP8266)
#include "platforms/esp/8266/spi_output_template.h"

#elif defined(NRF51)
#include "platforms/arm/nrf51/spi_output_template.h"

#elif defined(NRF52_SERIES)
#include "platforms/arm/nrf52/spi_output_template.h"

#elif defined(FASTLED_APOLLO3) && defined(FASTLED_ALL_PINS_HARDWARE_SPI)
#include "platforms/apollo3/spi_output_template.h"

#elif defined(FASTLED_TEENSY3) && defined(ARM_HARDWARE_SPI)
#include "platforms/arm/teensy/teensy3_common/spi_output_template.h"

#elif defined(FASTLED_TEENSY4) && defined(ARM_HARDWARE_SPI)
#include "platforms/arm/teensy/teensy4_common/spi_output_template.h"

#elif defined(FASTLED_TEENSYLC) && defined(ARM_HARDWARE_SPI)
#include "platforms/arm/teensy/teensy_lc/spi_output_template.h"

#elif defined(__SAM3X8E__) || defined(__SAMD21G18A__) || defined(__SAMD21J18A__) || defined(__SAMD21E17A__) || \
      defined(__SAMD21E18A__) || defined(__SAMD51G19A__) || defined(__SAMD51J19A__) || defined(__SAME51J19A__) || \
      defined(__SAMD51P19A__) || defined(__SAMD51P20A__)
#include "platforms/arm/sam/spi_output_template.h"

#elif defined(STM32F10X_MD) || defined(__STM32F1__) || defined(STM32F1) || defined(STM32F1xx) || defined(STM32F2XX) || defined(STM32F4)
#include "platforms/arm/stm32/spi_output_template.h"

#elif defined(AVR_HARDWARE_SPI) || defined(ARDUNIO_CORE_SPI)
#include "platforms/avr/spi_output_template.h"

#else
// Fallback: Generic software SPI for unsupported platforms
#include "platforms/shared/spi_bitbang/spi_output_template.h"
#endif

#ifndef FASTLED_CLOCKLESS_USES_NANOSECONDS
 #if defined(FASTLED_TEENSY4)
   #define FASTLED_CLOCKLESS_USES_NANOSECONDS 1
 #elif defined(ESP32)
   #include "third_party/espressif/led_strip/src/enabled.h"
   // RMT 5.1 driver converts from nanoseconds to RMT ticks.
   #if FASTLED_RMT5
	 #define FASTLED_CLOCKLESS_USES_NANOSECONDS 1
   #else
   	 #define FASTLED_CLOCKLESS_USES_NANOSECONDS 0
   #endif
 #else
   #define FASTLED_CLOCKLESS_USES_NANOSECONDS 0
 #endif  // FASTLED_TEENSY4
#endif  // FASTLED_CLOCKLESS_USES_NANOSECONDS


// Allow overclocking of the clockless family of leds. 1.2 would be
// 20% overclocking. In tests WS2812 can be overclocked at 20%, but
// various manufacturers may be different.  This is a global value
// which is overridable by each supported chipset.
#ifdef FASTLED_LED_OVERCLOCK
#warning "FASTLED_LED_OVERCLOCK has been changed to FASTLED_OVERCLOCK. Please update your code."
#define FASTLED_OVERCLOCK FASTLED_LED_OVERCLOCK
#endif

#ifndef FASTLED_OVERCLOCK
#define FASTLED_OVERCLOCK 1.0
#else
#ifndef FASTLED_OVERCLOCK_SUPPRESS_WARNING
#warning "FASTLED_OVERCLOCK is now active, #define FASTLED_OVERCLOCK_SUPPRESS_WARNING to disable this warning"
#endif
#endif

// So many platforms have specialized WS2812 controllers. Why? Because they
// are the cheapest chipsets use. So we special case this.
#include "platforms/chipsets_specialized_ws2812.h"

#if defined(ARDUINO) && (defined(SoftwareSerial_h) || defined(__SoftwareSerial_h))
#include <SoftwareSerial.h>
#endif

/// @file chipsets.h
/// Contains the bulk of the definitions for the various LED chipsets supported.



/// @defgroup Chipsets LED Chipset Controllers
/// Implementations of ::CLEDController classes for various led chipsets.
///
/// @{

#if defined(ARDUINO) && (defined(SoftwareSerial_h) || defined(__SoftwareSerial_h))

#define HAS_PIXIE


/// Adafruit Pixie controller class
/// @tparam DATA_PIN the pin to write data out on
/// @tparam RGB_ORDER the RGB ordering for the LED data
template<fl::u8 DATA_PIN, EOrder RGB_ORDER = RGB>
class PixieController : public CPixelLEDController<RGB_ORDER> {
	SoftwareSerial Serial;
	CMinWait<2000> mWait;

public:
	PixieController() : Serial(-1, DATA_PIN) {}

protected:
	/// Initialize the controller
	virtual void init() {
		Serial.begin(115200);
		mWait.mark();
	}

	/// @copydoc CPixelLEDController::showPixels()
	virtual void showPixels(PixelController<RGB_ORDER> & pixels) {
		mWait.wait();
		while(pixels.has(1)) {
			fl::u8 r = pixels.loadAndScale0();
			Serial.write(r);
			fl::u8 g = pixels.loadAndScale1();
			Serial.write(g);
			fl::u8 b = pixels.loadAndScale2();
			Serial.write(b);
			pixels.advanceData();
			pixels.stepDithering();
		}
		mWait.mark();
	}

};

// template<SoftwareSerial & STREAM, EOrder RGB_ORDER = RGB>
// class PixieController : public PixieBaseController<STREAM, RGB_ORDER> {
// public:
// 	virtual void init() {
// 		STREAM.begin(115200);
// 	}
// };

#endif
#endif

/// @brief Emulation layer to support RGBW LEDs on RGB controllers
/// @details This template class allows you to use RGBW (4-channel) LED strips with
/// controllers that only support RGB (3-channel) output. It works by:
/// 1. Creating an internal buffer to store the converted RGBW data
/// 2. Converting RGB color values to RGBW using configurable color conversion modes
/// 3. Packing the RGBW data (4 bytes per pixel) into RGB format (3 bytes) for transmission
/// 4. Sending the packed data to the underlying RGB controller
///
/// @tparam CONTROLLER The base RGB controller type (e.g., WS2812)
/// @tparam RGB_ORDER The color channel ordering (e.g., GRB for WS2812)
///
/// Usage Example:
/// @code
/// // Define your base RGB controller (must use RGB ordering, no reordering allowed)
/// typedef WS2812<DATA_PIN, RGB> ControllerT;
///
/// // Create the RGBW emulator with your desired color ordering
/// static RGBWEmulatedController<ControllerT, GRB> rgbwController;
///
/// // Add to FastLED
/// FastLED.addLeds(&rgbwController, leds, NUM_LEDS);
/// @endcode
///
/// Color Conversion Modes (via Rgbw parameter):
/// - kRGBWExactColors: Preserves color accuracy, reduces max brightness
/// - kRGBWMaxBrightness: Maximizes brightness, may oversaturate colors
/// - kRGBWBoostedWhite: Boosts white channel for better whites
/// - kRGBWNullWhitePixel: Disables white channel (RGB mode only)
///
/// @note The base CONTROLLER must use RGB ordering (no internal reordering).
/// Color channel reordering is handled by this wrapper class via RGB_ORDER.


template <
	typename CONTROLLER,
	EOrder RGB_ORDER = GRB>  // Default on WS2812>
class RGBWEmulatedController
    : public CPixelLEDController<RGB_ORDER, CONTROLLER::LANES_VALUE,
                                 CONTROLLER::MASK_VALUE> {
  public:
    // ControllerT is a helper class.  It subclasses the device controller class
    // and has three methods to call the three protected methods we use.
    // This is janky, but redeclaring public methods protected in a derived class
    // is janky, too.

    // N.B., byte order must be RGB.
	typedef CONTROLLER ControllerBaseT;
    class ControllerT : public CONTROLLER {
        friend class RGBWEmulatedController<CONTROLLER, RGB_ORDER>;
        void *callBeginShowLeds(int size) { return ControllerBaseT::beginShowLeds(size); }
        void callShow(CRGB *data, int nLeds, fl::u8 brightness) {
            ControllerBaseT::show(data, nLeds, brightness);
        }
        void callEndShowLeds(void *data) { ControllerBaseT::endShowLeds(data); }
    };


    static const int LANES = CONTROLLER::LANES_VALUE;
    static const uint32_t MASK = CONTROLLER::MASK_VALUE;

    // The delegated controller must do no reordering.
    static_assert(RGB == CONTROLLER::RGB_ORDER_VALUE, "The delegated controller MUST NOT do reordering");

    /// @brief Constructor with optional RGBW configuration
    /// @param rgbw Configuration for RGBW color conversion (defaults to kRGBWExactColors mode)
    RGBWEmulatedController(const Rgbw& rgbw = RgbwDefault()) {
        this->setRgbw(rgbw);
    };

    /// @brief Destructor - cleans up the internal RGBW buffer
    ~RGBWEmulatedController() { delete[] mRGBWPixels; }

	virtual void *beginShowLeds(int size) override {
		return mController.callBeginShowLeds(Rgbw::size_as_rgb(size));
	}

	virtual void endShowLeds(void *data) override {
		return mController.callEndShowLeds(data);
	}

    /// @brief Main rendering function that converts RGB to RGBW and shows pixels
    /// @details This function:
    /// 1. Converts each RGB pixel to RGBW format based on the configured conversion mode
    /// 2. Packs the RGBW data into a format the RGB controller can transmit
    /// 3. Temporarily bypasses color correction/temperature on the base controller
    /// 4. Sends the packed data to the physical LED strip
    /// @param pixels The pixel controller containing RGB data to be converted
    virtual void showPixels(PixelController<RGB_ORDER, LANES, MASK> &pixels) override {
        // Ensure buffer is large enough
        ensureBuffer(pixels.size());
		Rgbw rgbw = this->getRgbw();
        fl::u8 *data = fl::bit_cast_ptr<fl::u8>(mRGBWPixels);
        while (pixels.has(1)) {
            pixels.stepDithering();
            pixels.loadAndScaleRGBW(rgbw, data, data + 1, data + 2, data + 3);
            data += 4;
            pixels.advanceData();
        }

		// Force the device controller to a state where it passes data through
		// unmodified: color correction, color temperature, dither, and brightness
		// (passed as an argument to show()).  Temporarily enable the controller,
		// show the LEDs, and disable it again.
		//
		// The device controller is in the global controller list, so if we 
		// don't keep it disabled, it will refresh again with unknown brightness,
		// temperature, etc.

		mController.setCorrection(CRGB(255, 255, 255));
		mController.setTemperature(CRGB(255, 255, 255));
		mController.setDither(DISABLE_DITHER);

		mController.setEnabled(true);
		mController.callShow(mRGBWPixels, Rgbw::size_as_rgb(pixels.size()), 255);
		mController.setEnabled(false);
    }

  private:
    /// @brief Initialize the controller and disable the base controller
    /// @details The base controller is kept disabled to prevent it from
    /// refreshing with its own settings. We only enable it temporarily during show().
    void init() override {
		mController.init();
		mController.setEnabled(false);
	}

    /// @brief Ensures the internal RGBW buffer is large enough for the LED count
    /// @param num_leds Number of RGB LEDs to convert to RGBW
    /// @details Reallocates the buffer if needed, accounting for the 4:3 byte ratio
    /// when packing RGBW data into RGB format
    void ensureBuffer(int32_t num_leds) {
        if (num_leds != mNumRGBLeds) {
            mNumRGBLeds = num_leds;
            // The delegate controller expects the raw pixel byte data in multiples of 3.
            // In the case of src data not a multiple of 3, then we need to
            // add pad bytes so that the delegate controller doesn't walk off the end
            // of the array and invoke a buffer overflow panic.
            uint32_t new_size = Rgbw::size_as_rgb(num_leds);
            delete[] mRGBWPixels;
            mRGBWPixels = new CRGB[new_size];
			// showPixels may never clear the last two pixels.
			for (uint32_t i = 0; i < new_size; i++) {
				mRGBWPixels[i] = CRGB(0, 0, 0);
			}

			mController.setLeds(mRGBWPixels, new_size);
        }
    }

    CRGB *mRGBWPixels = nullptr;        ///< Internal buffer for packed RGBW data
    int32_t mNumRGBLeds = 0;            ///< Number of RGB LEDs in the original array
    int32_t mNumRGBWLeds = 0;           ///< Number of RGBW pixels the buffer can hold
    ControllerT mController;             ///< The underlying RGB controller instance
};

/// @defgroup ClockedChipsets Clocked Chipsets
/// Nominally SPI based, these chipsets have a data and a clock line.
/// @{

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// LPD8806 controller class - takes data/clock/select pin values (N.B. should take an SPI definition?)
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// LPD8806 controller class.
/// @tparam DATA_PIN the data pin for these LEDs
/// @tparam CLOCK_PIN the clock pin for these LEDs
/// @tparam RGB_ORDER the RGB ordering for these LEDs
/// @tparam SPI_SPEED the clock divider used for these LEDs.  Set using the ::DATA_RATE_MHZ / ::DATA_RATE_KHZ macros.  Defaults to ::DATA_RATE_MHZ(12)
template <int DATA_PIN, fl::u8 CLOCK_PIN, EOrder RGB_ORDER = RGB,  uint32_t SPI_SPEED = DATA_RATE_MHZ(12) >
class LPD8806Controller : public CPixelLEDController<RGB_ORDER> {
	typedef fl::SPIOutput<DATA_PIN, CLOCK_PIN, SPI_SPEED> SPI;

	class LPD8806_ADJUST {
	public:
		// LPD8806 spec wants the high bit of every rgb data byte sent out to be set.
		FASTLED_FORCE_INLINE static fl::u8 adjust(FASTLED_REGISTER fl::u8 data) { return ((data>>1) | 0x80) + ((data && (data<254)) & 0x01); }
		FASTLED_FORCE_INLINE static void postBlock(int len, void* context = nullptr) {
			SPI* pSPI = static_cast<SPI*>(context);
			pSPI->writeBytesValueRaw(0, ((len*3+63)>>6));
		}

	};

	SPI mSPI;

public:
	LPD8806Controller()  {}
	virtual void init() {
		mSPI.init();
	}

protected:

	/// @copydoc CPixelLEDController::showPixels()
	virtual void showPixels(PixelController<RGB_ORDER> & pixels) {
		fl::writePixelsToSPI<0, LPD8806_ADJUST, RGB_ORDER>(pixels, mSPI, &mSPI);
	}

public:
	/// Get the protocol-safe padding byte for LPD8806
	/// Used for quad-SPI lane padding when strips have different lengths
	/// @returns 0x00 (latch continuation byte)
	static constexpr fl::u8 getPaddingByte() { return 0x00; }

	/// Get a black LED frame for synchronized latching
	/// Used for quad-SPI lane padding to ensure all strips latch simultaneously
	/// @returns Black LED frame (invisible LED: GRB with MSB set)
	static fl::span<const fl::u8> getPaddingLEDFrame() {
		static const fl::u8 frame[] = {
			0x80,  // Green = 0 (with MSB=1)
			0x80,  // Red = 0 (with MSB=1)
			0x80   // Blue = 0 (with MSB=1)
		};
		return fl::span<const fl::u8>(frame, 3);
	}

	/// Get the size of the padding LED frame in bytes
	/// @returns 3 bytes per LED for LPD8806
	static constexpr size_t getPaddingLEDFrameSize() {
		return 3;
	}

	/// Calculate total byte count for LPD8806 protocol
	/// Used for quad-SPI buffer pre-allocation
	/// @param num_leds Number of LEDs in the strip
	/// @returns Total bytes needed (RGB data + latch bytes)
	static constexpr size_t calculateBytes(size_t num_leds) {
		// LPD8806 protocol:
		// - LED data: 3 bytes per LED (GRB with high bit set)
		// - Latch: ((num_leds * 3 + 63) / 64) bytes of 0x00
		return (num_leds * 3) + ((num_leds * 3 + 63) / 64);
	}
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// WS2801 definition - takes data/clock/select pin values (N.B. should take an SPI definition?)
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// WS2801 controller class.
/// @tparam DATA_PIN the data pin for these LEDs
/// @tparam CLOCK_PIN the clock pin for these LEDs
/// @tparam RGB_ORDER the RGB ordering for these LEDs
/// @tparam SPI_SPEED the clock divider used for these LEDs.  Set using the ::DATA_RATE_MHZ / ::DATA_RATE_KHZ macros.  Defaults to ::DATA_RATE_MHZ(1)
template <int DATA_PIN, fl::u8 CLOCK_PIN, EOrder RGB_ORDER = RGB, uint32_t SPI_SPEED = DATA_RATE_MHZ(1)>
class WS2801Controller : public CPixelLEDController<RGB_ORDER> {
	typedef fl::SPIOutput<DATA_PIN, CLOCK_PIN, SPI_SPEED> SPI;
	SPI mSPI;
	CMinWait<1000>  mWaitDelay;

public:
	WS2801Controller() {}

	/// Initialize the controller
	virtual void init() {
		mSPI.init();
	  mWaitDelay.mark();
	}

protected:

	/// @copydoc CPixelLEDController::showPixels()
	virtual void showPixels(PixelController<RGB_ORDER> & pixels) {
		mWaitDelay.wait();
		fl::writePixelsToSPI<0, DATA_NOP, RGB_ORDER>(pixels, mSPI, nullptr);
		mWaitDelay.mark();
	}

public:
	/// Get the protocol-safe padding byte for WS2801
	/// Used for quad-SPI lane padding when strips have different lengths
	/// @returns 0x00 (no protocol state)
	static constexpr fl::u8 getPaddingByte() { return 0x00; }

	/// Get a black LED frame for synchronized latching
	/// Used for quad-SPI lane padding to ensure all strips latch simultaneously
	/// @returns Black LED frame (invisible LED: RGB all zero)
	static fl::span<const fl::u8> getPaddingLEDFrame() {
		static const fl::u8 frame[] = {
			0x00,  // Red = 0
			0x00,  // Green = 0
			0x00   // Blue = 0
		};
		return fl::span<const fl::u8>(frame, 3);
	}

	/// Get the size of the padding LED frame in bytes
	/// @returns 3 bytes per LED for WS2801
	static constexpr size_t getPaddingLEDFrameSize() {
		return 3;
	}

	/// Calculate total byte count for WS2801 protocol
	/// Used for quad-SPI buffer pre-allocation
	/// @param num_leds Number of LEDs in the strip
	/// @returns Total bytes needed (RGB data only, no overhead)
	static constexpr size_t calculateBytes(size_t num_leds) {
		// WS2801 protocol:
		// - LED data: 3 bytes per LED (RGB)
		// - No frame overhead (latch is timing-based, not data-based)
		return num_leds * 3;
	}
};

/// WS2803 controller class.
/// @copydetails WS2801Controller
template <int DATA_PIN, fl::u8 CLOCK_PIN, EOrder RGB_ORDER = RGB, uint32_t SPI_SPEED = DATA_RATE_MHZ(25)>
class WS2803Controller : public WS2801Controller<DATA_PIN, CLOCK_PIN, RGB_ORDER, SPI_SPEED> {};

/// LPD6803 controller class (LPD1101).
/// 16 bit (1 bit const "1", 5 bit red, 5 bit green, 5 bit blue).
/// In chip CMODE pin must be set to 1 (inside oscillator mode).
/// @tparam DATA_PIN the data pin for these LEDs
/// @tparam CLOCK_PIN the clock pin for these LEDs
/// @tparam RGB_ORDER the RGB ordering for these LEDs
/// @tparam SPI_SPEED the clock divider used for these LEDs.  Set using the ::DATA_RATE_MHZ / ::DATA_RATE_KHZ macros.  Defaults to ::DATA_RATE_MHZ(12)
/// @see Datasheet: https://cdn-shop.adafruit.com/datasheets/LPD6803.pdf
template <int DATA_PIN, fl::u8 CLOCK_PIN, EOrder RGB_ORDER = RGB, uint32_t SPI_SPEED = DATA_RATE_MHZ(12)>
class LPD6803Controller : public CPixelLEDController<RGB_ORDER> {
	typedef fl::SPIOutput<DATA_PIN, CLOCK_PIN, SPI_SPEED> SPI;
	SPI mSPI;

	void startBoundary() { mSPI.writeByte(0); mSPI.writeByte(0); mSPI.writeByte(0); mSPI.writeByte(0); }
	void endBoundary(int nLeds) { int nDWords = (nLeds/32); do { mSPI.writeByte(0xFF); mSPI.writeByte(0x00); mSPI.writeByte(0x00); mSPI.writeByte(0x00); } while(nDWords--); }

public:
	LPD6803Controller() {}

	virtual void init() {
		mSPI.init();
	}

protected:
	/// @copydoc CPixelLEDController::showPixels()
	virtual void showPixels(PixelController<RGB_ORDER> & pixels) {
		mSPI.select();

		startBoundary();
		while(pixels.has(1)) {
            FASTLED_REGISTER fl::u16 command;
            command = 0x8000;
            command |= (pixels.loadAndScale0() & 0xF8) << 7; // red is the high 5 bits
            command |= (pixels.loadAndScale1() & 0xF8) << 2; // green is the middle 5 bits
			mSPI.writeByte((command >> 8) & 0xFF);
            command |= pixels.loadAndScale2() >> 3 ; // blue is the low 5 bits
			mSPI.writeByte(command & 0xFF);

			pixels.stepDithering();
			pixels.advanceData();
		}
		endBoundary(pixels.size());
		mSPI.waitFully();
		mSPI.release();
	}

};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// APA102 definition - takes data/clock/select pin values (N.B. should take an SPI definition?)
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// APA102 controller class.
/// @tparam DATA_PIN the data pin for these LEDs
/// @tparam CLOCK_PIN the clock pin for these LEDs
/// @tparam RGB_ORDER the RGB ordering for these LEDs
/// @tparam SPI_SPEED the clock divider used for these LEDs.  Set using the ::DATA_RATE_MHZ / ::DATA_RATE_KHZ macros.  Defaults to ::DATA_RATE_MHZ(12)
template <
	fl::u8 DATA_PIN, fl::u8 CLOCK_PIN,
	EOrder RGB_ORDER = RGB,
	// APA102 has a bug where long strip can't handle full speed due to clock degredation.
	// This only affects long strips, but then again if you have a short strip does 6 mhz actually slow
	// you down?  Probably not. And you can always bump it up for speed. Therefore we are prioritizing
	// "just works" over "fastest possible" here.
	// https://www.pjrc.com/why-apa102-leds-have-trouble-at-24-mhz/
	uint32_t SPI_SPEED = DATA_RATE_MHZ(6),
	fl::FiveBitGammaCorrectionMode GAMMA_CORRECTION_MODE = fl::kFiveBitGammaCorrectionMode_Null,
	uint32_t START_FRAME = 0x00000000,
	uint32_t END_FRAME = 0xFF000000
>
class APA102Controller : public CPixelLEDController<RGB_ORDER> {
	typedef fl::SPIOutput<DATA_PIN, CLOCK_PIN, SPI_SPEED> SPI;
	SPI mSPI;

	void startBoundary() {
		mSPI.writeWord(START_FRAME >> 16);
		mSPI.writeWord(START_FRAME & 0xFFFF);
	}
	void endBoundary(int nLeds) {
		int nDWords = (nLeds/32);
		const fl::u8 b0 = fl::u8(END_FRAME >> 24 & 0x000000ff);
		const fl::u8 b1 = fl::u8(END_FRAME >> 16 & 0x000000ff);
		const fl::u8 b2 = fl::u8(END_FRAME >>  8 & 0x000000ff);
		const fl::u8 b3 = fl::u8(END_FRAME >>  0 & 0x000000ff);
		do {
			mSPI.writeByte(b0);
			mSPI.writeByte(b1);
			mSPI.writeByte(b2);
			mSPI.writeByte(b3);
		} while(nDWords--);
	}

	FASTLED_FORCE_INLINE void writeLed(fl::u8 brightness, fl::u8 b0, fl::u8 b1, fl::u8 b2) {
#ifdef FASTLED_SPI_BYTE_ONLY
		mSPI.writeByte(0xE0 | brightness);
		mSPI.writeByte(b0);
		mSPI.writeByte(b1);
		mSPI.writeByte(b2);
#else
		fl::u16 b = 0xE000 | (brightness << 8) | (fl::u16)b0;
		mSPI.writeWord(b);
		fl::u16 w = b1 << 8;
		w |= b2;
		mSPI.writeWord(w);
#endif
	}

	FASTLED_FORCE_INLINE void write2Bytes(fl::u8 b1, fl::u8 b2) {
#ifdef FASTLED_SPI_BYTE_ONLY
		mSPI.writeByte(b1);
		mSPI.writeByte(b2);
#else
		mSPI.writeWord(fl::u16(b1) << 8 | b2);
#endif
	}

public:
	APA102Controller() {}

	virtual void init() override {
		mSPI.init();
	}

protected:
	/// @copydoc CPixelLEDController::showPixels()
	virtual void showPixels(PixelController<RGB_ORDER> & pixels) override {
		switch (GAMMA_CORRECTION_MODE) {
			case fl::kFiveBitGammaCorrectionMode_Null: {
				showPixelsDefault(pixels);
				break;
			}
			case fl::kFiveBitGammaCorrectionMode_BitShift: {
				showPixelsGammaBitShift(pixels);
				break;
			}
		}
	}

private:

	static inline void getGlobalBrightnessAndScalingFactors(
		    PixelController<RGB_ORDER> & pixels,
		    fl::u8* out_s0, fl::u8* out_s1, fl::u8* out_s2, fl::u8* out_brightness) {
#if FASTLED_HD_COLOR_MIXING
		fl::u8 brightness;
		pixels.loadRGBScaleAndBrightness(out_s0, out_s1, out_s2, &brightness);
		struct Math {
			static fl::u16 map(fl::u16 x, fl::u16 in_min, fl::u16 in_max, fl::u16 out_min, fl::u16 out_max) {
				const fl::u16 run = in_max - in_min;
				const fl::u16 rise = out_max - out_min;
				const fl::u16 delta = x - in_min;
				return (delta * rise) / run + out_min;
			}
		};
		// *out_brightness = Math::map(brightness, 0, 255, 0, 31);
		fl::u16 bri = Math::map(brightness, 0, 255, 0, 31);
		if (bri == 0 && brightness != 0) {
			// Fixes https://github.com/FastLED/FastLED/issues/1908
			bri = 1;
		}
		*out_brightness = static_cast<fl::u8>(bri);
		return;
#else
		fl::u8 s0, s1, s2;
		pixels.loadAndScaleRGB(&s0, &s1, &s2);
#if FASTLED_USE_GLOBAL_BRIGHTNESS == 1
		// This function is pure magic.
		const fl::u16 maxBrightness = 0x1F;
		fl::u16 brightness = ((((fl::u16)FL_MAX(FL_MAX(s0, s1), s2) + 1) * maxBrightness - 1) >> 8) + 1;
		s0 = (maxBrightness * s0 + (brightness >> 1)) / brightness;
		s1 = (maxBrightness * s1 + (brightness >> 1)) / brightness;
		s2 = (maxBrightness * s2 + (brightness >> 1)) / brightness;
#else
		const fl::u8 brightness = 0x1F;
#endif  // FASTLED_USE_GLOBAL_BRIGHTNESS
		*out_s0 = s0;
		*out_s1 = s1;
		*out_s2 = s2;
		*out_brightness = static_cast<fl::u8>(brightness);
#endif  // FASTLED_HD_COLOR_MIXING
	}

	// Legacy showPixels implementation.
	inline void showPixelsDefault(PixelController<RGB_ORDER> & pixels) {
		mSPI.select();
		fl::u8 s0, s1, s2, global_brightness;
		getGlobalBrightnessAndScalingFactors(pixels, &s0, &s1, &s2, &global_brightness);
		startBoundary();
		while (pixels.has(1)) {
			fl::u8 c0, c1, c2;
			pixels.loadAndScaleRGB(&c0, &c1, &c2);
			writeLed(global_brightness, c0, c1, c2);
			pixels.stepDithering();
			pixels.advanceData();
		}
		endBoundary(pixels.size());

		mSPI.waitFully();
		mSPI.release();

		// Finalize transmission (no-op on non-ESP32, flushes Quad-SPI on ESP32)
		mSPI.finalizeTransmission();
	}

	inline void showPixelsGammaBitShift(PixelController<RGB_ORDER> & pixels) {
		mSPI.select();
		startBoundary();
		while (pixels.has(1)) {
			// Load raw uncorrected r,g,b values.
			fl::u8 brightness, c0, c1, c2;  // c0-c2 is the RGB data re-ordered for pixel
			pixels.loadAndScale_APA102_HD(&c0, &c1, &c2, &brightness);
			writeLed(brightness, c0, c1, c2);
			pixels.stepDithering();
			pixels.advanceData();
		}
		endBoundary(pixels.size());
		mSPI.waitFully();
		mSPI.release();

		// Finalize transmission (no-op on non-ESP32, flushes Quad-SPI on ESP32)
		mSPI.finalizeTransmission();
	}

public:
	/// Get the protocol-safe padding byte for APA102
	/// Used for quad-SPI lane padding when strips have different lengths
	/// @returns 0xFF (end frame continuation byte)
	/// @deprecated Use getPaddingLEDFrame() for synchronized latching
	static constexpr fl::u8 getPaddingByte() { return 0xFF; }

	/// Get padding LED frame for synchronized latching in quad-SPI
	/// Returns a black LED frame to prepend to shorter strips, ensuring
	/// all strips finish transmitting simultaneously for synchronized updates
	/// @returns Black LED frame (4 bytes: brightness=0, RGB=0,0,0)
	static fl::span<const fl::u8> getPaddingLEDFrame() {
		// APA102 LED frame format: [111BBBBB][B][G][R]
		// Black LED: 0xE0 (brightness=0), RGB=0,0,0
		static const fl::u8 frame[] = {
			0xE0,  // Brightness byte (111 00000 = brightness 0)
			0x00,  // Blue = 0
			0x00,  // Green = 0
			0x00   // Red = 0
		};
		return fl::span<const fl::u8>(frame, 4);
	}

	/// Get size of padding LED frame in bytes
	/// @returns 4 (APA102 uses 4 bytes per LED)
	static constexpr size_t getPaddingLEDFrameSize() {
		return 4;
	}

	/// Calculate total byte count for APA102 protocol
	/// Used for quad-SPI buffer pre-allocation
	/// @param num_leds Number of LEDs in the strip
	/// @returns Total bytes needed (start frame + LED data + end frame)
	static constexpr size_t calculateBytes(size_t num_leds) {
		// APA102 protocol:
		// - Start frame: 4 bytes (0x00000000)
		// - LED data: 4 bytes per LED (brightness + RGB)
		// - End frame: (num_leds / 32) + 1 DWords = 4 * ((num_leds / 32) + 1) bytes
		return 4 + (num_leds * 4) + (4 * ((num_leds / 32) + 1));
	}
};

/// APA102 high definition controller class.
/// @tparam DATA_PIN the data pin for these LEDs
/// @tparam CLOCK_PIN the clock pin for these LEDs
/// @tparam RGB_ORDER the RGB ordering for these LEDs
/// @tparam SPI_SPEED the clock divider used for these LEDs.  Set using the ::DATA_RATE_MHZ / ::DATA_RATE_KHZ macros.  Defaults to ::DATA_RATE_MHZ(24)
template <
	fl::u8 DATA_PIN,
	fl::u8 CLOCK_PIN,
	EOrder RGB_ORDER = RGB,
	// APA102 has a bug where long strip can't handle full speed due to clock degredation.
	// This only affects long strips, but then again if you have a short strip does 6 mhz actually slow
	// you down?  Probably not. And you can always bump it up for speed. Therefore we are prioritizing
	// "just works" over "fastest possible" here.
	// https://www.pjrc.com/why-apa102-leds-have-trouble-at-24-mhz/
	uint32_t SPI_SPEED = DATA_RATE_MHZ(6)
>
class APA102ControllerHD : public APA102Controller<
	DATA_PIN,
	CLOCK_PIN, 
	RGB_ORDER,
	SPI_SPEED,
	fl::kFiveBitGammaCorrectionMode_BitShift,
	uint32_t(0x00000000),
	uint32_t(0x00000000)> {
public:
  APA102ControllerHD() = default;
  APA102ControllerHD(const APA102ControllerHD&) = delete;
};

/// SK9822 controller class. It's exactly the same as the APA102Controller protocol but with a different END_FRAME and default SPI_SPEED.
/// @tparam DATA_PIN the data pin for these LEDs
/// @tparam CLOCK_PIN the clock pin for these LEDs
/// @tparam RGB_ORDER the RGB ordering for these LEDs
/// @tparam SPI_SPEED the clock divider used for these LEDs.  Set using the ::DATA_RATE_MHZ / ::DATA_RATE_KHZ macros.  Defaults to ::DATA_RATE_MHZ(24)
template <
	fl::u8 DATA_PIN,
	fl::u8 CLOCK_PIN,
	EOrder RGB_ORDER = RGB,
	uint32_t SPI_SPEED = DATA_RATE_MHZ(12)
>
class SK9822Controller : public APA102Controller<
	DATA_PIN,
	CLOCK_PIN,
	RGB_ORDER,
	SPI_SPEED,
	fl::kFiveBitGammaCorrectionMode_Null,
	0x00000000,
	0x00000000
> {
};

/// SK9822 controller class. It's exactly the same as the APA102Controller protocol but with a different END_FRAME and default SPI_SPEED.
/// @tparam DATA_PIN the data pin for these LEDs
/// @tparam CLOCK_PIN the clock pin for these LEDs
/// @tparam RGB_ORDER the RGB ordering for these LEDs
/// @tparam SPI_SPEED the clock divider used for these LEDs.  Set using the ::DATA_RATE_MHZ / ::DATA_RATE_KHZ macros.  Defaults to ::DATA_RATE_MHZ(24)
template <
	fl::u8 DATA_PIN,
	fl::u8 CLOCK_PIN,
	EOrder RGB_ORDER = RGB,
	uint32_t SPI_SPEED = DATA_RATE_MHZ(12)
>
class SK9822ControllerHD : public APA102Controller<
	DATA_PIN,
	CLOCK_PIN,
	RGB_ORDER,
	SPI_SPEED,
	fl::kFiveBitGammaCorrectionMode_BitShift,
	0x00000000,
	0x00000000
> {
};


/// HD107 is just the APA102 with a default 40Mhz clock rate.
template <
	fl::u8 DATA_PIN,
	fl::u8 CLOCK_PIN,
	EOrder RGB_ORDER = RGB,
	uint32_t SPI_SPEED = DATA_RATE_MHZ(40)
>
class HD107Controller : public APA102Controller<
	DATA_PIN,
	CLOCK_PIN,
	RGB_ORDER,
	SPI_SPEED,
	fl::kFiveBitGammaCorrectionMode_Null,
	0x00000000,
	0x00000000
> {};

/// HD107HD is just the APA102HD with a default 40Mhz clock rate.
template <
	fl::u8 DATA_PIN,
	fl::u8 CLOCK_PIN,
	EOrder RGB_ORDER = RGB,
	uint32_t SPI_SPEED = DATA_RATE_MHZ(40)
>
class HD107HDController : public APA102ControllerHD<
	DATA_PIN,
	CLOCK_PIN,
	RGB_ORDER,
	SPI_SPEED> {
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// P9813 definition - takes data/clock/select pin values (N.B. should take an SPI definition?)
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// P9813 controller class.
/// @tparam DATA_PIN the data pin for these LEDs
/// @tparam CLOCK_PIN the clock pin for these LEDs
/// @tparam RGB_ORDER the RGB ordering for these LEDs
/// @tparam SPI_SPEED the clock divider used for these LEDs.  Set using the ::DATA_RATE_MHZ / ::DATA_RATE_KHZ macros.  Defaults to ::DATA_RATE_MHZ(10)
template <int DATA_PIN, fl::u8 CLOCK_PIN, EOrder RGB_ORDER = RGB, uint32_t SPI_SPEED = DATA_RATE_MHZ(10)>
class P9813Controller : public CPixelLEDController<RGB_ORDER> {
	typedef fl::SPIOutput<DATA_PIN, CLOCK_PIN, SPI_SPEED> SPI;
	SPI mSPI;

	void writeBoundary() { mSPI.writeWord(0); mSPI.writeWord(0); }

	FASTLED_FORCE_INLINE void writeLed(fl::u8 r, fl::u8 g, fl::u8 b) {
		FASTLED_REGISTER fl::u8 top = 0xC0 | ((~b & 0xC0) >> 2) | ((~g & 0xC0) >> 4) | ((~r & 0xC0) >> 6);
		mSPI.writeByte(top); mSPI.writeByte(b); mSPI.writeByte(g); mSPI.writeByte(r);
	}

public:
	P9813Controller() {}

	virtual void init() {
		mSPI.init();
	}

protected:
	/// @copydoc CPixelLEDController::showPixels()
	virtual void showPixels(PixelController<RGB_ORDER> & pixels) {
		mSPI.select();

		writeBoundary();
		while(pixels.has(1)) {
			writeLed(pixels.loadAndScale0(), pixels.loadAndScale1(), pixels.loadAndScale2());
			pixels.advanceData();
			pixels.stepDithering();
		}
		writeBoundary();
		mSPI.waitFully();

		mSPI.release();
	}

public:
	/// Get the protocol-safe padding byte for P9813
	/// Used for quad-SPI lane padding when strips have different lengths
	/// @returns 0x00 (boundary byte)
	static constexpr fl::u8 getPaddingByte() { return 0x00; }

	/// Get a black LED frame for synchronized latching
	/// Used for quad-SPI lane padding to ensure all strips latch simultaneously
	/// @returns Black LED frame (invisible LED: flag byte + BGR all zero)
	static fl::span<const fl::u8> getPaddingLEDFrame() {
		static const fl::u8 frame[] = {
			0xFF,  // Flag byte for RGB=0,0,0
			0x00,  // Blue = 0
			0x00,  // Green = 0
			0x00   // Red = 0
		};
		return fl::span<const fl::u8>(frame, 4);
	}

	/// Get the size of the padding LED frame in bytes
	/// @returns 4 bytes per LED for P9813
	static constexpr size_t getPaddingLEDFrameSize() {
		return 4;
	}

	/// Calculate total byte count for P9813 protocol
	/// Used for quad-SPI buffer pre-allocation
	/// @param num_leds Number of LEDs in the strip
	/// @returns Total bytes needed (boundaries + LED data)
	static constexpr size_t calculateBytes(size_t num_leds) {
		// P9813 protocol:
		// - Start boundary: 4 bytes (0x00000000)
		// - LED data: 4 bytes per LED (flag byte + BGR)
		// - End boundary: 4 bytes (0x00000000)
		return 4 + (num_leds * 4) + 4;
	}
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// SM16716 definition - takes data/clock/select pin values (N.B. should take an SPI definition?)
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// SM16716 controller class.
/// @tparam DATA_PIN the data pin for these LEDs
/// @tparam CLOCK_PIN the clock pin for these LEDs
/// @tparam RGB_ORDER the RGB ordering for these LEDs
/// @tparam SPI_SPEED the clock divider used for these LEDs.  Set using the ::DATA_RATE_MHZ / ::DATA_RATE_KHZ macros.  Defaults to ::DATA_RATE_MHZ(16)
template <int DATA_PIN, fl::u8 CLOCK_PIN, EOrder RGB_ORDER = RGB, uint32_t SPI_SPEED = DATA_RATE_MHZ(16)>
class SM16716Controller : public CPixelLEDController<RGB_ORDER> {
	typedef fl::SPIOutput<DATA_PIN, CLOCK_PIN, SPI_SPEED> SPI;
	SPI mSPI;

	void writeHeader() {
		// Write out 50 zeros to the spi line (6 blocks of 8 followed by two single bit writes)
		mSPI.select();
		mSPI.template writeBit<0>(0);
		mSPI.writeByte(0);
		mSPI.writeByte(0);
		mSPI.writeByte(0);
		mSPI.template writeBit<0>(0);
		mSPI.writeByte(0);
		mSPI.writeByte(0);
		mSPI.writeByte(0);
		// Note: waitFully() and release() may not be strictly necessary for SM16716
		// since we're just streaming bytes. However, they're kept here for consistency
		// with other SPI-based controllers and as defensive programming.
		mSPI.waitFully();
		mSPI.release();
	}

public:
	SM16716Controller() {}

	virtual void init() {
		mSPI.init();
	}

protected:
	/// @copydoc CPixelLEDController::showPixels()
	virtual void showPixels(PixelController<RGB_ORDER> & pixels) {
		// Make sure the FLAG_START_BIT flag is set to ensure that an extra 1 bit is sent at the start
		// of each triplet of bytes for rgb data
		// writeHeader();
		fl::writePixelsToSPI<FLAG_START_BIT, DATA_NOP, RGB_ORDER>(pixels, mSPI, nullptr);
		writeHeader();
	}

};

/// @} ClockedChipsets


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Clockless template instantiations - see clockless.h for how the timing values are used
//

#ifdef FASTLED_HAS_CLOCKLESS
/// @defgroup ClocklessChipsets Clockless Chipsets
/// These chipsets have only a single data line.
///
/// The clockless chipset controllers use the same base class
/// and the same protocol, but with varying timing periods.
///
/// These controllers have 3 control points in their cycle for each bit:
///   @code
///   At T=0        : the line is raised hi to start a bit
///   At T=T1       : the line is dropped low to transmit a zero bit
///   At T=T1+T2    : the line is dropped low to transmit a one bit
///   At T=T1+T2+T3 : the cycle is concluded (next bit can be sent)
///   @endcode
///
/// The units used for T1, T2, and T3 is nanoseconds.
///
/// For 8MHz/16MHz/24MHz frequencies, these values are also guaranteed
/// to be integral multiples of an 8MHz clock (125ns increments).
///
/// @note The base class, ClocklessController, is platform-specific.
/// @note Centralized timing definitions are available in:
///   - fl::chipsets::led_timing.h - Nanosecond-based T1, T2, T3 definitions for all chipsets
///   - fl::platforms::avr::led_timing_legacy_avr.h - Legacy AVR FMUL-based definitions (backward compat)
/// @{

// Allow clock that clockless controller is based on to have different
// frequency than the CPU.
#if !defined(CLOCKLESS_FREQUENCY)
    #define CLOCKLESS_FREQUENCY F_CPU
#endif

// We want to force all avr's to use the Trinket controller when running at 8Mhz, because even the 328's at 8Mhz
// need the more tightly defined timeframes.
#if defined(__LGT8F__) || (CLOCKLESS_FREQUENCY == 8000000 || CLOCKLESS_FREQUENCY == 16000000 || CLOCKLESS_FREQUENCY == 24000000) || defined(FASTLED_DOXYGEN) //  || CLOCKLESS_FREQUENCY == 48000000 || CLOCKLESS_FREQUENCY == 96000000) // 125ns/clock

/// Frequency multiplier for each clockless data interval.
/// @see Notes in @ref ClocklessChipsets
#define FMUL (CLOCKLESS_FREQUENCY/8000000)

/// @note AVR FMUL-based controllers (below) use platform-specific timing optimizations
/// that have been validated on AVR processors at 8/16/24MHz. These values represent
/// tested approximations for AVR platforms and should NOT be changed to match the
/// nanosecond values in fl::chipsets::led_timing.h, as they may introduce timing
/// regressions. For reference timing values, see fl::TIMING_* constants in led_timing.h.

// Suppress -Wsubobject-linkage warning for controller template instantiations.
// In C++11/14, constexpr variables have internal linkage, which causes the compiler
// to warn when they are used in base classes of externally-linked templates. The macro
// FL_INLINE_CONSTEXPR adds explicit 'inline' in C++17+ to resolve this, but for C++11
// compatibility we suppress the warning here.
FL_DISABLE_WARNING_PUSH
FL_DISABLE_WARNING(subobject-linkage)

/// GE8822 controller class.
/// @copydetails WS2812Controller800Khz
/// @note Timing: 350ns, 660ns, 350ns (nanosecond-based timing)
/// @see fl::TIMING_GE8822_800KHZ in fl::chipsets::led_timing.h
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class GE8822Controller800Khz : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_GE8822_800KHZ, RGB_ORDER, 4> {};

/// LPD1886 controller class.
/// @copydetails WS2812Controller800Khz
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class LPD1886Controller1250Khz : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_LPD1886_1250KHZ, RGB_ORDER, 4> {};

/// LPD1886 controller class.
/// @copydetails WS2812Controller800Khz
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class LPD1886Controller1250Khz_8bit : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_LPD1886_1250KHZ, RGB_ORDER> {};

#if !FASTLED_WS2812_HAS_SPECIAL_DRIVER
/// WS2812 controller class @ 800 KHz.
/// @tparam DATA_PIN the data pin for these LEDs
/// @tparam RGB_ORDER the RGB ordering for these LEDs
/// @note Timing: 250ns, 625ns, 375ns (nanosecond-based timing)
/// @see fl::TIMING_WS2812_800KHZ in fl::chipsets::led_timing.h
template <int DATA_PIN, EOrder RGB_ORDER = GRB>
class WS2812Controller800Khz : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_WS2812_800KHZ, RGB_ORDER> {};
#endif

/// WS2815 controller class @ 400 KHz.
/// @copydetails WS2812Controller800Khz
template <int DATA_PIN, EOrder RGB_ORDER = GRB>
class WS2815Controller : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_WS2815, RGB_ORDER> {};

/// DP1903 controller class @ 800 KHz.
/// @copydetails WS2812Controller800Khz
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class DP1903Controller800Khz : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_DP1903_800KHZ, RGB_ORDER> {};

/// DP1903 controller class @ 400 KHz.
/// @copydetails WS2812Controller800Khz
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class DP1903Controller400Khz : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_DP1903_400KHZ, RGB_ORDER> {};

/// WS2813 controller class.
/// @copydetails WS2812Controller800Khz
template <int DATA_PIN, EOrder RGB_ORDER = GRB>                                                             //not tested
class WS2813Controller : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_WS2813, RGB_ORDER> {};

/// WS2811 controller class @ 400 KHz.
/// @copydetails WS2812Controller800Khz
template <int DATA_PIN, EOrder RGB_ORDER = GRB>
class WS2811Controller400Khz : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_WS2811_400KHZ, RGB_ORDER> {};

/// WS2811 controller class @ 800 KHz.
/// @copydetails WS2812Controller800Khz
template <int DATA_PIN, EOrder RGB_ORDER = GRB>
class WS2811Controller800Khz : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_WS2811_800KHZ_LEGACY, RGB_ORDER> {};

/// SK6822 controller class.
/// @copydetails WS2812Controller800Khz
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class SK6822Controller : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_SK6822, RGB_ORDER> {};

/// SM16703 controller class.
/// @copydetails WS2812Controller800Khz
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class SM16703Controller : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_SM16703, RGB_ORDER> {};

/// SK6812 controller class.
/// @copydetails WS2812Controller800Khz
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class SK6812Controller : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_SK6812, RGB_ORDER> {};

/// UCS1903 controller class @ 400 KHz.
/// @copydetails WS2812Controller800Khz
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class UCS1903Controller400Khz : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_UCS1903_400KHZ, RGB_ORDER> {};

/// UCS1903B controller class.
/// @copydetails WS2812Controller800Khz
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class UCS1903BController800Khz : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_UCS1903B_800KHZ, RGB_ORDER> {};

/// UCS1904 controller class.
/// @copydetails WS2812Controller800Khz
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class UCS1904Controller800Khz : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_UCS1904_800KHZ, RGB_ORDER> {};

/// UCS2903 controller class.
/// @copydetails WS2812Controller800Khz
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class UCS2903Controller : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_UCS2903, RGB_ORDER> {};

/// TM1809 controller class.
/// @copydetails WS2812Controller800Khz
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class TM1809Controller800Khz : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_TM1809_800KHZ, RGB_ORDER> {};

/// TM1803 controller class.
/// @copydetails WS2812Controller800Khz
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class TM1803Controller400Khz : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_TM1803_400KHZ, RGB_ORDER> {};

/// TM1829 controller class.
/// @copydetails WS2812Controller800Khz
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class TM1829Controller800Khz : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_TM1829_800KHZ, RGB_ORDER, 0, true> {};

/// GW6205 controller class @ 400 KHz.
/// @copydetails WS2812Controller800Khz
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class GW6205Controller400Khz : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_GW6205_400KHZ, RGB_ORDER, 4> {};

/// UCS1904 controller class @ 800 KHz.
/// @copydetails WS2812Controller800Khz
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class GW6205Controller800Khz : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_GW6205_800KHZ, RGB_ORDER, 4> {};

/// PL9823 controller class.
/// @copydetails WS2812Controller800Khz
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class PL9823Controller : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_PL9823, RGB_ORDER> {};

// UCS1912 - Note, never been tested, this is according to the datasheet
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class UCS1912Controller : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_UCS1912, RGB_ORDER> {};

/// SM16824E controller class.
/// @copydetails WS2812Controller800Khz
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class SM16824EController : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_SM16824E, RGB_ORDER> {};



// UCS7604 controller typedefs (available on supported platforms)
// Uses WS2812-like timing (800kHz) for preambles and pixel data by default
#ifdef UCS7604_HAS_CONTROLLER
/// UCS7604 controller class @ 800 KHz, 16-bit mode
/// @tparam DATA_PIN the data pin for these LEDs
/// @tparam RGB_ORDER the RGB ordering for these LEDs (typically GRB for UCS7604)
///
/// The UCS7604 is a 4-channel (RGBW) LED driver with 16-bit color resolution.
/// This controller sends protocol-required preambles before pixel data.
/// Default mode: 16-bit depth @ 800 kHz with RGBW ordering.
template <int DATA_PIN, EOrder RGB_ORDER = GRB>
class UCS7604Controller800Khz : public fl::UCS7604Controller<DATA_PIN, fl::TIMING_UCS7604_800KHZ, RGB_ORDER, fl::UCS7604_MODE_16BIT_800KHZ> {};

/// UCS7604 controller class @ 800 KHz, 8-bit mode
/// @copydetails UCS7604Controller800Khz
template <int DATA_PIN, EOrder RGB_ORDER = GRB>
class UCS7604Controller800Khz_8bit : public fl::UCS7604Controller<DATA_PIN, fl::TIMING_UCS7604_800KHZ, RGB_ORDER, fl::UCS7604_MODE_8BIT_800KHZ> {};

/// UCS7604 default typedef (16-bit @ 800kHz)
template <int DATA_PIN, EOrder RGB_ORDER = GRB>
using UCS7604 = UCS7604Controller800Khz<DATA_PIN, RGB_ORDER>;
#endif

#else



// WS2812 can be overclocked pretty aggressively, however, there are
// some excellent articles that you should read about WS2812 overclocking
// and corruption for a large number of LEDs.
// https://wp.josh.com/2014/05/16/why-you-should-give-your-neopixel-bits-room-to-breathe/
// https://wp.josh.com/2014/05/13/ws2812-neopixels-are-not-so-finicky-once-you-get-to-know-them/
#ifndef FASTLED_OVERCLOCK_WS2812
#define FASTLED_OVERCLOCK_WS2812 FASTLED_OVERCLOCK
#endif

#ifndef FASTLED_OVERCLOCK_WS2811
#define FASTLED_OVERCLOCK_WS2811 FASTLED_OVERCLOCK
#endif

#ifndef FASTLED_OVERCLOCK_WS2813
#define FASTLED_OVERCLOCK_WS2813 FASTLED_OVERCLOCK
#endif

#ifndef FASTLED_OVERCLOCK_WS2815
#define FASTLED_OVERCLOCK_WS2815 FASTLED_OVERCLOCK
#endif

#ifndef FASTLED_OVERCLOCK_SK6822
#define FASTLED_OVERCLOCK_SK6822 FASTLED_OVERCLOCK
#endif

#ifndef FASTLED_OVERCLOCK_SK6812
#define FASTLED_OVERCLOCK_SK6812 FASTLED_OVERCLOCK
#endif

/// Calculates the number of cycles for the clockless chipset (which may differ from CPU cycles)
/// @see ::NS()
#if FASTLED_CLOCKLESS_USES_NANOSECONDS
// just use raw nanosecond values for the teensy4
#define C_NS(_NS) _NS
#else
#define C_NS(_NS) (((_NS * ((CLOCKLESS_FREQUENCY / 1000000L)) + 999)) / 1000)
#endif

// Allow overclocking various LED chipsets in the clockless family.
// Clocked chips like the APA102 don't need this because they allow
// you to control the clock speed directly.
#define C_NS_WS2812(_NS) (C_NS(int(_NS / FASTLED_OVERCLOCK_WS2812)))
#define C_NS_WS2811(_NS) (C_NS(int(_NS / FASTLED_OVERCLOCK_WS2811)))
#define C_NS_WS2813(_NS) (C_NS(int(_NS / FASTLED_OVERCLOCK_WS2813)))
#define C_NS_WS2815(_NS) (C_NS(int(_NS / FASTLED_OVERCLOCK_WS2815)))
#define C_NS_SK6822(_NS) (C_NS(int(_NS / FASTLED_OVERCLOCK_SK6822)))
#define C_NS_SK6812(_NS) (C_NS(int(_NS / FASTLED_OVERCLOCK_SK6812)))



// At T=0        : the line is raised hi to start a bit
// At T=T1       : the line is dropped low to transmit a zero bit
// At T=T1+T2    : the line is dropped low to transmit a one bit
// At T=T1+T2+T3 : the cycle is concluded (next bit can be sent)
//
// Python script to calculate the values for T1, T2, and T3 for FastLED:
// Note: there is a discussion on whether this python script is correct or not:
//  https://github.com/FastLED/FastLED/issues/1806
//
//  print("Enter the values of T0H, T0L, T1H, T1L, in nanoseconds: ")
//  T0H = int(input("  T0H: "))
//  T0L = int(input("  T0L: "))
//  T1H = int(input("  T1H: "))
//  T1L = int(input("  T1L: "))
//  
//  duration = max(T0H + T0L, T1H + T1L)
//  
//  print("The max duration of the signal is: ", duration)
//  
//  T1 = T0H
//  T2 = T1H
//  T3 = duration - T0H - T0L
//  
//  print("T1: ", T1)
//  print("T2: ", T2)
//  print("T3: ", T3)


/// GE8822 controller @ 800 kHz - references centralized timing from fl::TIMING_GE8822_800KHZ
/// @see fl::TIMING_GE8822_800KHZ in fl::chipsets::led_timing.h (350, 660, 350 ns)
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class GE8822Controller800Khz : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_GE8822_800KHZ, RGB_ORDER, 4, false> {};

/// GW6205 controller @ 400 kHz - references centralized timing from fl::TIMING_GW6205_400KHZ
/// @see fl::TIMING_GW6205_400KHZ in fl::chipsets::led_timing.h (800, 800, 800 ns)
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class GW6205Controller400Khz : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_GW6205_400KHZ, RGB_ORDER, 4, false> {};

/// GW6205 controller @ 800 kHz - references centralized timing from fl::TIMING_GW6205_800KHZ
/// @see fl::TIMING_GW6205_800KHZ in fl::chipsets::led_timing.h (400, 400, 400 ns)
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class GW6205Controller800Khz : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_GW6205_800KHZ, RGB_ORDER> {};

/// UCS1903 controller @ 400 kHz - references centralized timing from fl::TIMING_UCS1903_400KHZ
/// @see fl::TIMING_UCS1903_400KHZ in fl::chipsets::led_timing.h (500, 1500, 500 ns)
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class UCS1903Controller400Khz : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_UCS1903_400KHZ, RGB_ORDER> {};

/// UCS1903B controller @ 800 kHz - references centralized timing from fl::TIMING_UCS1903B_800KHZ
/// @see fl::TIMING_UCS1903B_800KHZ in fl::chipsets::led_timing.h (400, 450, 450 ns)
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class UCS1903BController800Khz : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_UCS1903B_800KHZ, RGB_ORDER> {};

/// UCS1904 controller @ 800 kHz - references centralized timing from fl::TIMING_UCS1904_800KHZ
/// @see fl::TIMING_UCS1904_800KHZ in fl::chipsets::led_timing.h (400, 400, 450 ns)
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class UCS1904Controller800Khz : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_UCS1904_800KHZ, RGB_ORDER> {};

/// UCS2903 controller - references centralized timing from fl::TIMING_UCS2903
/// @see fl::TIMING_UCS2903 in fl::chipsets::led_timing.h (250, 750, 250 ns)
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class UCS2903Controller : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_UCS2903, RGB_ORDER> {};

/// TM1809 controller @ 800 kHz - references centralized timing from fl::TIMING_TM1809_800KHZ
/// @see fl::TIMING_TM1809_800KHZ in fl::chipsets::led_timing.h (350, 350, 450 ns)
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class TM1809Controller800Khz : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_TM1809_800KHZ, RGB_ORDER> {};

/// WS2811 controller @ 800 kHz - references centralized timing from fl::TIMING_WS2811_800KHZ_LEGACY
/// @see fl::TIMING_WS2811_800KHZ_LEGACY in fl::chipsets::led_timing.h (500, 2000, 2000 ns)
template <int DATA_PIN, EOrder RGB_ORDER = GRB>
class WS2811Controller800Khz : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_WS2811_800KHZ_LEGACY, RGB_ORDER> {};

/// WS2813 controller - references centralized timing from fl::TIMING_WS2813
/// @see fl::TIMING_WS2813 in fl::chipsets::led_timing.h (320, 320, 640 ns)
template <int DATA_PIN, EOrder RGB_ORDER = GRB>
class WS2813Controller : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_WS2813, RGB_ORDER> {};

#ifndef FASTLED_WS2812_T1
#define FASTLED_WS2812_T1 250
#endif

#ifndef FASTLED_WS2812_T2
#define FASTLED_WS2812_T2 625
#endif

#ifndef FASTLED_WS2812_T3
#define FASTLED_WS2812_T3 375
#endif



/// WS2812 controller @ 800 kHz - references centralized timing from fl::TIMING_WS2812_800KHZ
/// @note Timing: 250ns, 625ns, 375ns (overclockable via FASTLED_OVERCLOCK_WS2812)
/// @see fl::TIMING_WS2812_800KHZ in chipsets::led_timing.h (250, 625, 375 ns)
/// @note Timings can be overridden at compile time using FASTLED_WS2812_T1, FASTLED_WS2812_T2, FASTLED_WS2812_T3
#if !FASTLED_WS2812_HAS_SPECIAL_DRIVER
template <int DATA_PIN, EOrder RGB_ORDER = GRB>
class WS2812Controller800Khz : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_WS2812_800KHZ, RGB_ORDER> {};
#endif

/// WS2811 controller @ 400 kHz - references centralized timing from fl::TIMING_WS2811_400KHZ
/// @see fl::TIMING_WS2811_400KHZ in chipsets::led_timing.h (800, 800, 900 ns)
template <int DATA_PIN, EOrder RGB_ORDER = GRB>
class WS2811Controller400Khz : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_WS2811_400KHZ, RGB_ORDER> {};

/// WS2815 controller - references centralized timing from fl::TIMING_WS2815
/// @see fl::TIMING_WS2815 in chipsets::led_timing.h (250, 1090, 550 ns)
template <int DATA_PIN, EOrder RGB_ORDER = GRB>
class WS2815Controller : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_WS2815, RGB_ORDER> {};

/// TM1803 controller @ 400 kHz - references centralized timing from fl::TIMING_TM1803_400KHZ
/// @see fl::TIMING_TM1803_400KHZ in chipsets::led_timing.h (700, 1100, 700 ns)
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class TM1803Controller400Khz : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_TM1803_400KHZ, RGB_ORDER> {};

/// TM1829 controller @ 800 kHz - references centralized timing from fl::TIMING_TM1829_800KHZ
/// @see fl::TIMING_TM1829_800KHZ in chipsets::led_timing.h (340, 340, 550 ns)
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class TM1829Controller800Khz : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_TM1829_800KHZ, RGB_ORDER> {};

/// TM1829 controller @ 1600 kHz - references centralized timing from fl::TIMING_TM1829_1600KHZ
/// @see fl::TIMING_TM1829_1600KHZ in chipsets::led_timing.h (100, 300, 200 ns)
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class TM1829Controller1600Khz : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_TM1829_1600KHZ, RGB_ORDER> {};

/// LPD1886 controller @ 1250 kHz - references centralized timing from fl::TIMING_LPD1886_1250KHZ
/// @see fl::TIMING_LPD1886_1250KHZ in chipsets::led_timing.h (200, 400, 200 ns)
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class LPD1886Controller1250Khz : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_LPD1886_1250KHZ, RGB_ORDER> {};

/// LPD1886 controller @ 1250 kHz (8-bit) - references centralized timing from fl::TIMING_LPD1886_1250KHZ
/// @see fl::TIMING_LPD1886_1250KHZ in chipsets::led_timing.h (200, 400, 200 ns)
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class LPD1886Controller1250Khz_8bit : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_LPD1886_1250KHZ, RGB_ORDER> {};


/// SK6822 controller - references centralized timing from fl::TIMING_SK6822
/// @see fl::TIMING_SK6822 in chipsets::led_timing.h (375, 1000, 375 ns)
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class SK6822Controller : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_SK6822, RGB_ORDER> {};

/// SK6812 controller - references centralized timing from fl::TIMING_SK6812
/// @see fl::TIMING_SK6812 in chipsets::led_timing.h (300, 600, 300 ns)
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class SK6812Controller : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_SK6812, RGB_ORDER> {};

/// SM16703 controller - references centralized timing from fl::TIMING_SM16703
/// @see fl::TIMING_SM16703 in chipsets::led_timing.h (300, 600, 300 ns)
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class SM16703Controller : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_SM16703, RGB_ORDER> {};

/// PL9823 controller - references centralized timing from fl::TIMING_PL9823
/// @see fl::TIMING_PL9823 in chipsets::led_timing.h (350, 1010, 350 ns)
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class PL9823Controller : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_PL9823, RGB_ORDER> {};

/// UCS1912 controller - references centralized timing from fl::TIMING_UCS1912
/// @note Never been tested, this is according to the datasheet
/// @see fl::TIMING_UCS1912 in chipsets::led_timing.h (250, 1000, 350 ns)
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class UCS1912Controller : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_UCS1912, RGB_ORDER> {};

/// SM16824E controller - references centralized timing from fl::TIMING_SM16824E
/// @note NEW LED! Help us test it! Under development.
/// Timing: T0H=.3μs, T0L=.9μs, T1H=.9μs, T1L=.3μs, TRST=200μs
/// @see fl::TIMING_SM16824E in chipsets::led_timing.h (300, 900, 100 ns)
template <int DATA_PIN, EOrder RGB_ORDER = RGB>
class SM16824EController : public fl::ClocklessControllerImpl<DATA_PIN, fl::TIMING_SM16824E, RGB_ORDER> {};
#endif
FL_DISABLE_WARNING_POP
/// @} ClocklessChipsets


// WS2816 - is an emulated controller that emits 48 bit pixels by forwarding
// them to a platform specific WS2812 controller.  The WS2812 controller
// has to output twice as many 24 bit pixels.
template <int DATA_PIN, EOrder RGB_ORDER = GRB>
class WS2816Controller
    : public CPixelLEDController<RGB_ORDER, 
                                 WS2812Controller800Khz<DATA_PIN, RGB>::LANES_VALUE,
                                 WS2812Controller800Khz<DATA_PIN, RGB>::MASK_VALUE> {

public:

	// ControllerT is a helper class.  It subclasses the device controller class
	// and has three methods to call the three protected methods we use.
	// This is janky, but redeclaring public methods protected in a derived class
	// is janky, too.

    // N.B., byte order must be RGB.
	typedef WS2812Controller800Khz<DATA_PIN, RGB> ControllerBaseT;
	class ControllerT : public ControllerBaseT {
		friend class WS2816Controller<DATA_PIN, RGB_ORDER>;
		void *callBeginShowLeds(int size) { return ControllerBaseT::beginShowLeds(size); }
		void callShow(CRGB *data, int nLeds, fl::u8 brightness) {
			ControllerBaseT::show(data, nLeds, brightness);
		}
		void callEndShowLeds(void *data) { ControllerBaseT::endShowLeds(data); }
	};

    static const int LANES = ControllerT::LANES_VALUE;
    static const uint32_t MASK = ControllerT::MASK_VALUE;

    WS2816Controller() {}
    ~WS2816Controller() {
        mController.setLeds(nullptr, 0);
        delete [] mData;
    }

    virtual void *beginShowLeds(int size) override {
        mController.setEnabled(true);
		void *result = mController.callBeginShowLeds(2 * size);
        mController.setEnabled(false);
        return result;
    }

    virtual void endShowLeds(void *data) override {
        mController.setEnabled(true);
		mController.callEndShowLeds(data);
        mController.setEnabled(false);
    }

    virtual void showPixels(PixelController<RGB_ORDER, LANES, MASK> &pixels) override {
        // Ensure buffer is large enough
        ensureBuffer(pixels.size());

		// expand and copy all the pixels
		size_t out_index = 0;
        while (pixels.has(1)) {
            pixels.stepDithering();

			fl::u16 s0, s1, s2;
            pixels.loadAndScale_WS2816_HD(&s0, &s1, &s2);
			fl::u8 b0_hi = s0 >> 8;
			fl::u8 b0_lo = s0 & 0xFF;
			fl::u8 b1_hi = s1 >> 8;
			fl::u8 b1_lo = s1 & 0xFF;
			fl::u8 b2_hi = s2 >> 8;
			fl::u8 b2_lo = s2 & 0xFF;

			mData[out_index] = CRGB(b0_hi, b0_lo, b1_hi);
			mData[out_index + 1] = CRGB(b1_lo, b2_hi, b2_lo);

            pixels.advanceData();
			out_index += 2;
        }

		// ensure device controller won't modify color values
        mController.setCorrection(CRGB(255, 255, 255));
        mController.setTemperature(CRGB(255, 255, 255));
        mController.setDither(DISABLE_DITHER);

		// output the data stream
        mController.setEnabled(true);
#ifdef BOUNCE_SUBCLASS
		mController.callShow(mData, 2 * pixels.size(), 255);
#else
        mController.show(mData, 2 * pixels.size(), 255);
#endif
        mController.setEnabled(false);
    }

private:
    void init() override {
        mController.init();
        mController.setEnabled(false);
    }

    void ensureBuffer(int size_8bit) {
        int size_16bit = 2 * size_8bit;
        if (mController.size() != size_16bit) {
            delete [] mData;
            CRGB *new_leds = new CRGB[size_16bit];
			mData = new_leds;
            mController.setLeds(new_leds, size_16bit);
        }
    }

    CRGB *mData = 0;
    ControllerT mController;
};


#endif

/// @defgroup SilabsChipsets Silicon Labs ezWS2812 Controllers
/// Hardware-accelerated LED controllers for Silicon Labs MGM240/MG24 series
///
/// These controllers use Silicon Labs' ezWS2812 drivers to provide optimized
/// WS2812 LED control on MGM240 and MG24 series microcontrollers.
///
/// Available controllers:
/// - EZWS2812_SPI: Uses hardware SPI (must define FASTLED_USES_EZWS2812_SPI)
/// - EZWS2812_GPIO: Uses optimized GPIO timing (always available)
/// @{

#if defined(ARDUINO_ARCH_SILABS)

#include "platforms/arm/mgm240/clockless_ezws2812_gpio.h"

/// Silicon Labs ezWS2812 GPIO controller (always available)
/// @tparam DATA_PIN the pin to write data out on
/// @tparam RGB_ORDER the RGB ordering for these LEDs (typically GRB for WS2812)
///
/// This controller uses optimized GPIO manipulation with frequency-specific timing.
/// Automatically selects 39MHz or 78MHz implementation based on F_CPU.
///
/// Usage:
/// @code
/// FastLED.addLeds<EZWS2812_GPIO, 7, GRB>(leds, NUM_LEDS);
/// @endcode
template<fl::u8 DATA_PIN, EOrder RGB_ORDER = GRB>
using EZWS2812_GPIO = fl::ClocklessController_ezWS2812_GPIO_Auto<DATA_PIN, RGB_ORDER>;

#ifdef FASTLED_USES_EZWS2812_SPI

#include "platforms/arm/mgm240/clockless_ezws2812_spi.h"

/// Silicon Labs ezWS2812 SPI controller (requires FASTLED_USES_EZWS2812_SPI)
/// @tparam RGB_ORDER the RGB ordering for these LEDs (typically GRB for WS2812)
///
/// This controller uses the MGM240/MG24's hardware SPI peripheral to generate
/// precise WS2812 timing signals. Excellent performance but consumes SPI peripheral.
///
/// IMPORTANT: You must define FASTLED_USES_EZWS2812_SPI before including FastLED.h
///
/// Usage:
/// @code
/// #define FASTLED_USES_EZWS2812_SPI
/// #include <FastLED.h>
///
/// void setup() {
///     FastLED.addLeds<EZWS2812_SPI, GRB>(leds, NUM_LEDS);
/// }
/// @endcode
template<EOrder RGB_ORDER = GRB>
using EZWS2812_SPI = fl::ClocklessController_ezWS2812_SPI<RGB_ORDER>;

#endif // FASTLED_USES_EZWS2812_SPI

#endif // ARDUINO_ARCH_SILABS

/// @} SilabsChipsets

/// @} Chipsets
