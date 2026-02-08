//============================================================= (c) A.Kolesov ==
// Управление освежителем воздуха. (29-01-2024)
// Доработка под ch32V003, TA6586 и вместо фотоэлемента в качестве датчика
// включения света испольуется напряжение с платы заряда аккумулятора.
// Если свет включен - измеряется время включенного состояния.
// При переходе из включенного в выключенное состояние проверяется измеренное
// время.
// Если во включенном состоянии находились более 5мин, делается три пшика.
// После этого происходит переход в спящее состояние с пробуждением по включению света.
// В состоянии, когда свет включен, можно пшикнуть кнопкой на морде, если она
// имеется.
//------------------------------------------------------------------------------
#include <Driver_TA6586.h>
#include <Logs.h>
#include <SysClock.h>
#include <ch32Pins.hpp>
#include <debug.h>
#include <stdint.h>

#define pinPress PC1   // Выход нажатия на распылитель
#define pinRelease PC2 // Выход возврата распылителя в исходное положение
// #define pinLed PD7     // Светодиод на корпусе
#define pinLed PD4     // Светодиод на корпусе
#define pinLight PC3   // Вход датчика освещенности (13кОм на землю, датчик к +3.3в)
#define pinBtn PC4     // Кнопка на корпусе

#define sittingTime 240000  // 4*60*1000 Время сидения, после которого пшикаем
#define shortSitTime 120000 // 2*60*1000 Короткое время (для жены)
// #define sittingTime 5000 // для отладки

#define ON 1
#define OFF 0
#define HIGH 1
#define LOW 0

#define LED_ON() pinWrite(pinLed, HIGH)    // Включить светодиод
#define LED_OFF() pinWrite(pinLed, LOW)    // Погасить светодиод
#define bttnPressed pinRead(pinBtn) == LOW // Кнопка нажата

void pshik(uint8_t cnt);    // Пшикаем
void ledBlink(uint8_t cnt); // Мигнем светодиодом
void system_sleep(void);    // Переход в спячку

uint32_t lightOn = 0; // Момент включения света

Driver drv(pinPress, pinRelease);

#ifdef __cplusplus
extern "C" {
#endif
void EXTI7_0_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void EXTI7_0_IRQHandler(void) {
  if (EXTI_GetITStatus(extiLine(pinLight)) != RESET) { // Проснулись по датчику света
    EXTI_ClearITPendingBit(extiLine(pinLight));        // Очистка флага прерывания
  }
  if (EXTI_GetITStatus(extiLine(pinBtn)) != RESET) { // Проснулись по кнопке
    EXTI_ClearITPendingBit(extiLine(pinBtn));        // Очистка флага прерывания
  }
}
#ifdef __cplusplus
}
#endif

int main() {

  SystemCoreClockUpdate();
#ifdef LOG_ENABLE
  USART_Printf_Init(115200);
#endif
  logs("SystemClk: %lu\r\n", SystemCoreClock);        // Для посмотреть частоту процесора (48мГц)
  logs("   ChipID: 0x%08lX\r\n", DBGMCU_GetCHIPID()); // Для посмотреть ID чипа, от нефиг делать

  // Настройка выводов контроллера - драйвер свои пины сам настроит
  // pinMode(pinPress, GPIO_Mode_Out_PP);
  // pinWrite(pinPress, LOW);
  // pinMode(pinRelease, GPIO_Mode_Out_PP);
  // pinWrite(pinRelease, LOW);

  pinMode(pinLed, GPIO_Mode_Out_PP);
  LED_OFF();

  pinMode(pinBtn, GPIO_Mode_IPU);
  pinExtiInit(pinBtn, EXTI_Trigger_Falling); // Подключаем прерывание - просыпаемся по кнопке
  pinMode(pinLight, GPIO_Mode_IPU);
  pinExtiInit(pinLight, EXTI_Trigger_Falling); // Подключаем прерывание - просыпаемся по свету

  // При включении помигаем светодиодом, показывая, что включаемся
  ledBlink(5);

  // Если включаемся без подключения к сети, то уйдем в спячку.
  // Если же сеть/свет есть, то уйдем в спячку и сразу проснемся.
  logs("Sleep on start...\r\n");
  system_sleep(); // Сразу в сон после включения
  // pshik(5);
  logs("Wake up...\r\n");

  bool lightState = OFF;
  while (1) {

    bool prevState = lightState;     // Предыдущее состояние света
    lightState = !pinRead(pinLight); // Текущее состояние света
    // Если состояние света изменилось (свет был выключен и включился, или наоборот)
    if (lightState != prevState) {
      prevState = lightState;
      uint32_t now = millis(); // Момент изменения состояния света
      if (lightState == ON) {  // Свет включили. Начнем измерять время включенного состояния
        lightOn = now;         // Момент включения
        LED_ON();
      } else { // Свет выключили
        if (lightOn != 0) {
          if (now - lightOn > sittingTime) { // Свет горел дольше заданного значения, нужно пшикать
            ledBlink(1);
            pshik(4);
          } else {
            if (now - lightOn > shortSitTime) { // Свет горел по меньшему порогу, нужно пшикать, но меньше
              ledBlink(1);
              pshik(2);
            }
          }
        }
        // И тут засыпаем, т.к. свет уже погас и пшики сделаны (если должны были)
        lightOn = 0;
        ledBlink(3);
        system_sleep();
      }
    }

    // При каждом нажатии кнопки один раз мигаем и пшикаем.
    // Кнопка обрабатывается при включенном свете, а при выключенном мы спим.
    if (bttnPressed) {
      ledBlink(1);
      pshik(1);
      LED_ON();
    }
  }
}

//==============================================================================
// Функция пшикает указанное число раз. Для этого мотор включается на некоторое
// время в одну сторону, затем в другую.
//------------------------------------------------------------------------------
void pshik(uint8_t cnt) {
  const uint16_t pressTime = 1000;   // Время включения двигателя для нажатия
  const uint16_t releaseTime = 1000; // Время включения двигателя на отпускание
  const uint16_t pause = 200;        // Пауза после нажатия перед отпусканием
  const uint16_t pauseBetween = 500; // Пауза между пшиками

  // pinWrite(pinRelease, LOW); // Это чтобы перебдеть
  for (uint8_t i = 0; i < cnt; i++) {
    // Нажатие
    // pinWrite(pinPress, HIGH);
    logs("Press...\r\n");
    drv.forward();
    delay(pressTime);
    // pinWrite(pinPress, LOW);
    drv.stop();
    logs("Hold...\r\n");

    delay(pause); // Пауза перед отпусканием

    // Отпускание
    // pinWrite(pinRelease, HIGH);
    logs("Release...\r\n");
    drv.reward();
    delay(releaseTime);
    // pinWrite(pinRelease, LOW);
    drv.stop();
    logs("Stop...\r\n");
    if (i < cnt - 1) {
      delay(pauseBetween); // Пауза после пшика (перед следующим), если он не последний
    }
  }
}

// Мигнем светодиодом указанное число раз
void ledBlink(uint8_t cnt) {

  for (uint8_t i = 0; i < cnt; i++) {
    logs("Blink...\r\n");
    LED_ON();
    delay(200);
    LED_OFF();
    delay(200);
  }
}

// Переход в спячку с пробуждением по датчику света или нажатию кнопки
void system_sleep() {
  Sysclock.end();
  __WFI();
  Sysclock.begin();
}
