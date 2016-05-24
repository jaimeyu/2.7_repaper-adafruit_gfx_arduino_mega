// Copyright 2013-2015 Pervasive Displays, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at:
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
// express or implied.  See the License for the specific language
// governing permissions and limitations under the License.

#if !defined(EPD_V110_G1_H)
#define EPD_V110_G1_H 1

#include <Arduino.h>
#include <SPI.h>

#if defined(__AVR__)
#include <avr/pgmspace.h>
#else
#ifndef PROGMEM
#define PROGMEM
#endif
#endif

// compile-time #if configuration
#define EPD_CHIP_VERSION      1
#define EPD_FILM_VERSION      110
#define EPD_PWM_REQUIRED      1
#define EPD_IMAGE_ONE_ARG     1
#define EPD_IMAGE_TWO_ARG     0
#define EPD_PARTIAL_AVAILABLE 0

// display panels supported
#define EPD_1_44_SUPPORT      1
#define EPD_1_9_SUPPORT       0
#define EPD_2_0_SUPPORT       1
#define EPD_2_6_SUPPORT       0
#define EPD_2_7_SUPPORT       1


// if more SRAM available (8 kBytes)
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
#define EPD_ENABLE_EXTRA_SRAM 1
#endif

typedef enum {
	EPD_1_44,        // 128 x 96
	EPD_2_0,         // 200 x 96
	EPD_2_7          // 264 x 176
} EPD_size;

typedef enum {           // Image pixel -> Display pixel
	EPD_compensate,  // B -> W, W -> B (Current Image)
	EPD_white,       // B -> N, W -> W (Current Image)
	EPD_inverse,     // B -> N, W -> B (New Image)
	EPD_normal       // B -> B, W -> W (New Image)
} EPD_stage;

// just for compatibility with othe newe panels
typedef enum {           // error codes
	EPD_OK,
	EPD_UNSUPPORTED_COG,
	EPD_PANEL_BROKEN,
	EPD_DC_FAILED
} EPD_error;

typedef void EPD_reader(void *buffer, uint32_t address, uint16_t length);

class EPD_Class {
private:
	const uint8_t EPD_Pin_PANEL_ON;
	const uint8_t EPD_Pin_BORDER;
	const uint8_t EPD_Pin_DISCHARGE;
	const uint8_t EPD_Pin_PWM;
	const uint8_t EPD_Pin_RESET;
	const uint8_t EPD_Pin_BUSY;
	const uint8_t EPD_Pin_EPD_CS;

	const EPD_size size;
	uint16_t base_stage_time;
	uint16_t factored_stage_time;
	uint16_t lines_per_display;
	uint16_t bytes_per_line;
	uint16_t bytes_per_scan;
	PROGMEM const uint8_t *gate_source;
	uint16_t gate_source_length;
	PROGMEM const uint8_t *channel_select;
	uint16_t channel_select_length;

	bool filler;

	EPD_Class(const EPD_Class &f);  // prevent copy

public:
	// power up and power down the EPD panel
	void begin(void);
	void end(void);

	void setFactor(int temperature = 25) {
		this->factored_stage_time = this->base_stage_time * this->temperature_to_factor_10x(temperature) / 10;
	}

	// there is no status available from the hardware
	const bool operator!(void) const {
		return false;  // EPD_OK != this->status;
	}

	// there is no status available from the hardware
	EPD_error error(void) const {
		return EPD_OK; // this->status;
	}

	// clear display (anything -> white)
	void clear(void) {
//		this->frame_fixed_repeat(0xff, EPD_compensate);
//		Serial.println("Cleared 0xff EPD_compensate");
//		this->frame_fixed_repeat(0xff, EPD_white);
//		Serial.println("Cleared 0xff  EPD_white ");
//		this->frame_fixed_repeat(0xaa, EPD_inverse);
//		Serial.println("Cleared 0xaa EPD_inverse");
		this->frame_fixed_repeat(0xaa, EPD_normal);
		Serial.println("Cleared display EPD_normal");
	}
#if EPD_IMAGE_TWO_ARG

	// assuming a clear (white) screen output an image (PROGMEM data)
	void image_0(PROGMEM const uint8_t *image) {
		this->frame_fixed_repeat(0xaa, EPD_compensate);
		this->frame_fixed_repeat(0xaa, EPD_white);
		this->frame_data_repeat(image, EPD_inverse);
		this->frame_data_repeat(image, EPD_normal);
	}

	// change from old image to new image (PROGMEM data)
	void image(PROGMEM const uint8_t *old_image, PROGMEM const uint8_t *new_image) {
		this->frame_data_repeat(old_image, EPD_compensate);
		this->frame_data_repeat(old_image, EPD_white);
		this->frame_data_repeat(new_image, EPD_inverse);
		this->frame_data_repeat(new_image, EPD_normal);
	}
#else
	// change from old image to new image (PROGMEM data)
		void image(PROGMEM const uint8_t *image) {
			clear();
//			this->frame_fixed_repeat(0xaa, EPD_compensate);
//			Serial.println("image 0xff EPD_compensate");
//
//			this->frame_fixed_repeat(0xaa, EPD_white);
//			Serial.println("image 0xff  EPD_white ");
//			this->frame_data_repeat(image, EPD_inverse);
//			Serial.println("image 0xaa EPD_inverse");
			this->frame_data_repeat(image, EPD_normal);
			Serial.println("image display EPD_normal");
		}
#endif

#if defined(EPD_ENABLE_EXTRA_SRAM)

	// change from old image to new image (SRAM version)
	void image_sram(const uint8_t *old_image, const uint8_t *new_image) {
		this->frame_sram_repeat(old_image, EPD_compensate);
		this->frame_sram_repeat(old_image, EPD_white);
		this->frame_sram_repeat(new_image, EPD_inverse);
		this->frame_sram_repeat(new_image, EPD_normal);
	}
#endif

	// Low level API calls
	// ===================

	// single frame refresh
	void frame_fixed(uint8_t fixed_value, EPD_stage stage);
	void frame_data(PROGMEM const uint8_t *new_image, EPD_stage stage);
#if defined(EPD_ENABLE_EXTRA_SRAM)
	void frame_sram(const uint8_t *new_image, EPD_stage stage);
#endif
	void frame_cb(uint32_t address, EPD_reader *reader, EPD_stage stage);

	// stage_time frame refresh
	void frame_fixed_repeat(uint8_t fixed_value, EPD_stage stage);
#if EPD_IMAGE_TWO_ARG
	void frame_data_repeat(PROGMEM const uint8_t *new_image, EPD_stage stage);
#else
	void frame_data_repeat(const uint8_t *new_image, EPD_stage stage);
#endif
#if defined(EPD_ENABLE_EXTRA_SRAM)
	void frame_sram_repeat(const uint8_t *new_image, EPD_stage stage);
#endif
	void frame_cb_repeat(uint32_t address, EPD_reader *reader, EPD_stage stage);

	// convert temperature to compensation factor
	int temperature_to_factor_10x(int temperature) const;

	// single line display - very low-level
	// also has to handle AVR progmem
	void line(uint16_t line, const uint8_t *data, uint8_t fixed_value, bool read_progmem, EPD_stage stage);

	// inline static void attachInterrupt();
	// inline static void detachInterrupt();

	EPD_Class(EPD_size _size,
		  uint8_t panel_on_pin,
		  uint8_t border_pin,
		  uint8_t discharge_pin,
		  uint8_t pwm_pin,
		  uint8_t reset_pin,
		  uint8_t busy_pin,
		  uint8_t chip_select_pin);

};

#endif
