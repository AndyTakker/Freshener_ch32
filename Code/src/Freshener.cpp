//============================================================= (c) A.Kolesov ==
// Управление освежителем воздуха. (29-01-2024)
// Доработка под ch32V003, TA6586 и вместо фотоэлемента в качестве датчика
// включения света используется напряжение с входа платы заряда аккумулятора.
// Если свет включен - измеряется время включенного состояния.
// При переходе из включенного в выключенное состояние проверяется измеренное
// время.
// Если во включенном состоянии находились более 5мин, делается четыре пшика,
// если от двух до пяти минут - два пшика.
// После этого происходит переход в спящее состояние с пробуждением по включению света.
// В состоянии, когда свет включен, можно пшикнуть кнопкой на морде, если она
// имеется.
//------------------------------------------------------------------------------
#include <Freshener.h>

uint32_t lightOn = 0; // Момент включения света

Driver drv(Pins::Press, Pins::Release);

#ifdef __cplusplus
extern "C" {
#endif
void EXTI7_0_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast"), used, section(".text.irq")));
void EXTI7_0_IRQHandler(void) {
  if (EXTI_GetITStatus(extiLine(Pins::Light)) != RESET) { // Проснулись по датчику света
    EXTI_ClearITPendingBit(extiLine(Pins::Light));        // Очистка флага прерывания
  }
  if (EXTI_GetITStatus(extiLine(Pins::Btn)) != RESET) { // Проснулись по кнопке
    EXTI_ClearITPendingBit(extiLine(Pins::Btn));        // Очистка флага прерывания
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

  pinMode(Pins::Led, GPIO_Mode_Out_PP); // Пин светодиода (на PD7 почему-то не заработал)
  LED_OFF();

  pinMode(Pins::Btn, GPIO_Mode_IPU);
  pinExtiInit(Pins::Btn, EXTI_Trigger_Falling); // Подключаем прерывание - просыпаемся по кнопке
  pinMode(Pins::Light, GPIO_Mode_IPU);
  pinExtiInit(Pins::Light, EXTI_Trigger_Falling); // Подключаем прерывание - просыпаемся по свету

  // При включении помигаем светодиодом, показывая, что включаемся
  ledBlink(5);

  // Если включаемся без подключения к сети, то уйдем в спячку.
  // Если же сеть/свет есть, то уйдем в спячку и сразу проснемся.
  logs("Sleep on start...\r\n");
  system_sleep(); // Сразу в сон после включения
  logs("Wake up...\r\n");

  bool lightState = OFF;
  while (1) {
    delay(100);
    bool prevState = lightState;        // Предыдущее состояние света
    lightState = !pinRead(Pins::Light); // Текущее состояние света
    // Если состояние света изменилось (свет был выключен и включился, или наоборот)
    if (lightState != prevState) {
      prevState = lightState;
      uint32_t now = millis(); // Момент изменения состояния света
      if (lightState == ON) {  // Свет включили. Начнем измерять время включенного состояния
        lightOn = now;         // Момент включения
        LED_ON();
      } else { // Свет выключили
        if (lightOn != 0) {
          logs("Light on: %lu\r\n", now - lightOn);
          if (now - lightOn > sittingTime) { // Свет горел дольше заданного значения, нужно пшикать
            ledBlink(1);
            pshik(longCnt);
          } else {
            if (now - lightOn > shortSitTime) { // Свет горел по меньшему порогу, нужно пшикать, но меньше
              ledBlink(1);
              pshik(shortCnt);
            }
          }
        }
        // И тут засыпаем, т.к. свет уже погас и пшики сделаны (если должны были)
        lightOn = 0;
        ledBlink(3);
        logs("Sleep...\r\n");
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
  constexpr uint16_t pressTime = 1000;   // Время включения двигателя для нажатия
  constexpr uint16_t releaseTime = 1000; // Время включения двигателя на отпускание
  constexpr uint16_t pause = 200;        // Пауза после нажатия перед отпусканием
  constexpr uint16_t pauseBetween = 500; // Пауза между пшиками

  for (uint8_t i = 0; i < cnt; i++) {
    // Нажатие
    logs("Press...\r\n");
    drv.forward();
    delay(pressTime);
    drv.stop();
    logs("Hold...\r\n");

    delay(pause); // Пауза перед отпусканием

    // Отпускание
    logs("Release...\r\n");
    drv.reward();
    delay(releaseTime);
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
