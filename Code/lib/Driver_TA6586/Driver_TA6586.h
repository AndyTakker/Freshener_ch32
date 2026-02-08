// Библиотека для управления двигателем постоянного тока.
// Вкл/выкл двигателя управляется транзистором, а смена направления движения - реле.
//
#pragma once

#include <stdint.h>
#include <SysClock.h>
#include <ch32Pins.hpp>

#define BRAKING_DELAY 100   // Пауза в миллисекундах для торможения двигателя
#define HIGH 1
#define LOW 0

enum drvState {
    Stopped,                // Остановлен
    Braking,                // Тормозит
    Forward,                // Движение вперед
    Reward                  // Движение назад
};

class Driver
{
private:
    PinName pinFw;          // Выход контроллера для включения вращения вперед
    PinName pinRw;          // Выход контроллера для включения вращения назад
    drvState state;         // Состояние двигателя

public:

    Driver(PinName pinDriverFw, PinName pinDriverRw);
    void forward(void);                             // Включение двигателя вперед
    void reward(void);                              // Включение двигателя назад
    void stop(void);                                // Отключить питание двигателя
    void braking(void);                             // Торможение двигателя
    drvState getState(void);                        // Получить статус двигателя
};
