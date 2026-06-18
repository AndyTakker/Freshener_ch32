#pragma once

#include "HwConfig.h"
#include <Driver_TA6586.h>
#include <Logs.h>
#include <SysClock.h>
#include <ch32Pins.hpp>
#include <debug.h>
#include <stdint.h>

#define ON 1
#define OFF 0
#define HIGH 1
#define LOW 0

#define LED_ON() pinWrite(Pins::Led, HIGH)    // Включить светодиод
#define LED_OFF() pinWrite(Pins::Led, LOW)    // Погасить светодиод
#define bttnPressed pinRead(Pins::Btn) == LOW // Кнопка нажата

void pshik(uint8_t cnt);    // Пшикаем
void ledBlink(uint8_t cnt); // Мигнем светодиодом
void system_sleep(void);    // Переход в спячку