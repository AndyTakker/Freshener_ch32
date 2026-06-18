#pragma once

#include <ch32Pins.hpp>

namespace Pins {
  PinName Press = PC1;   // Выход нажатия на распылитель
  PinName Release = PC2; // Выход возврата распылителя в исходное положение
  PinName Led = PD4;     // Светодиод на корпусе (на PD7 почему-то не заработал)
  PinName Light = PC3;   // Вход датчика освещенности
  PinName Btn = PC4;     // Кнопка на корпусе
}

#ifndef LIGHT_DEBUG
constexpr uint32_t sittingTime = 240000; // 4*60*1000 Время сидения, после которого пшикаем
#else
constexpr uint32_t sittingTime = 5000; // 4*60*1000 Время сидения, после которого пшикаем
#endif
constexpr uint32_t shortSitTime = 120000; // 2*60*1000 Короткое время (для жены)

constexpr uint8_t longCnt = 4;  // Количество пшиков при долгом сидении
constexpr uint8_t shortCnt = 2; // Количество пшиков при коротком сидении
