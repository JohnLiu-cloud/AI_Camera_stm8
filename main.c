#include "stm8s.h"
#include "stm8s_tim1.h"
#include "stm8s_gpio.h"
#include "stm8s_adc1.h"
#if TEST_ENABLED
#define GPIO_TEST      GPIOC
#define GPIO_TEST_PIN  GPIO_PIN_6
#define TEST_REVERSE()    GPIO_WriteReverse( GPIO_TEST , (GPIO_Pin_TypeDef)GPIO_TEST_PIN )  
#endif

void Adc_Init( void );
void Adc_Get( int16_t *p_value );

int16_t Adc_Value;

void Power_On(void);
void Power_OFF(void);
void Power_Shundown(void);

#define GPIO_SHUNT     GPIOC
#define GPIO_SHUNT_PIN GPIO_PIN_3


#define GPIO_STATE     GPIOD
#define GPIO_STATE_PIN GPIO_PIN_5
#define STATE_ON       GPIO_WriteHigh(GPIO_STATE,GPIO_STATE_PIN)
#define STATE_OFF      GPIO_WriteLow(GPIO_STATE,GPIO_STATE_PIN)

#define GPIO_DISCHG GPIOC
#define GPIO_DISCHG_PIN GPIO_PIN_6
#define DISCHG_ON       GPIO_WriteHigh(GPIO_DISCHG,GPIO_DISCHG_PIN)
#define DISCHG_OFF      GPIO_WriteLow(GPIO_DISCHG,GPIO_DISCHG_PIN)

#define GPIO_BOOST     GPIOC
#define GPIO_BOOST_PIN GPIO_PIN_4
#define BOOST_ON       GPIO_WriteHigh(GPIO_BOOST,GPIO_BOOST_PIN)
#define BOOST_OFF      GPIO_WriteLow(GPIO_BOOST,GPIO_BOOST_PIN)

#define GPIO_SWITCH     GPIOD
#define GPIO_SWITCH_PIN GPIO_PIN_3
#define SWITCH_ON       GPIO_WriteHigh(GPIO_SWITCH,GPIO_SWITCH_PIN)
#define SWITCH_OFF      GPIO_WriteLow(GPIO_SWITCH,GPIO_SWITCH_PIN)

#define SYSTEM_ON  0x01
#define SYSTEM_OFF 0x00
#define SHUNT_COUNT_TIMEOUT 30
uint16_t poweron_count;
uint16_t poweroff_count;
uint8_t  system_power;
void shunt_detection_fun(void)
{
  if( GPIO_ReadInputPin( GPIO_SHUNT , GPIO_SHUNT_PIN ) ) {
      poweron_count++;
      poweroff_count=0;
      if( poweron_count >= SHUNT_COUNT_TIMEOUT ) {
          poweron_count = SHUNT_COUNT_TIMEOUT;
          Power_OFF();
      }
  } else {
      poweron_count = 0;
      poweroff_count++;
      if( poweroff_count>SHUNT_COUNT_TIMEOUT ) {
          poweroff_count = SHUNT_COUNT_TIMEOUT;
          Power_On();
      }
      
  }
}

static void clk_init(void);
static void Time1_Init(void);
static void gpio_Init(void);

#pragma vector=0xD
__interrupt void TIM1_UPD_OVF_TRG_BRK_IRQHandler(void)
{
  static uint16_t timer = 0;
#if TEST_ENABLED  
  TEST_REVERSE();
#endif
  shunt_detection_fun();
  timer++;
  if( timer%20 == 0 ) { //Timer 200ms
      Adc_Get( &Adc_Value );
      if( system_power == SYSTEM_OFF ) {
        if( Adc_Value < 630 ) { // 2.1v adc 630
            Power_Shundown();
        }
      }
  } 
  if( timer%100 == 0 ) { //Timer 1s
  
  }
  if( timer >= 60000 ) {
      timer = 0;
  }
  TIM1_ClearITPendingBit(TIM1_IT_UPDATE);
}

int main( void )
{
  clk_init();
  gpio_Init();
  system_power = SYSTEM_OFF;
  Power_On();
  Adc_Init();
  Time1_Init();
  enableInterrupts();
  for( ;; ) {
  
  }
  //return 0;
}

static void clk_init(void)
{
   CLK_DeInit();
   CLK_HSICmd( DISABLE );
   CLK_HSECmd( ENABLE );
   CLK_ClockSecuritySystemEnable();
   CLK_ClockSwitchConfig(CLK_SWITCHMODE_AUTO,CLK_SOURCE_HSE,DISABLE,CLK_CURRENTCLOCKSTATE_ENABLE);
}

static void Time1_Init(void)
{
  TIM1_TimeBaseInit(16,TIM1_COUNTERMODE_UP,1150,0);
  TIM1_ITConfig(TIM1_IT_UPDATE , ENABLE);
  TIM1_ARRPreloadConfig(ENABLE);
  TIM1_Cmd(ENABLE);
}

static void gpio_Init(void)
{
#if TEST_ENABLED
  GPIO_Init(GPIO_TEST,(GPIO_Pin_TypeDef)GPIO_TEST_PIN, GPIO_MODE_OUT_PP_LOW_FAST);
#endif 
  GPIO_Init(GPIO_SHUNT,(GPIO_Pin_TypeDef)GPIO_SHUNT_PIN, GPIO_MODE_IN_PU_NO_IT);
  GPIO_Init(GPIO_DISCHG,(GPIO_Pin_TypeDef)GPIO_DISCHG_PIN, GPIO_MODE_OUT_PP_LOW_FAST);
  GPIO_Init(GPIO_BOOST,(GPIO_Pin_TypeDef)GPIO_BOOST_PIN, GPIO_MODE_OUT_PP_LOW_FAST);
  GPIO_Init(GPIO_SWITCH,(GPIO_Pin_TypeDef)GPIO_SWITCH_PIN, GPIO_MODE_OUT_PP_LOW_FAST);
  GPIO_Init(GPIO_STATE,(GPIO_Pin_TypeDef)GPIO_STATE_PIN, GPIO_MODE_OUT_PP_LOW_FAST);
}
void Power_On(void)
{
  if( system_power != SYSTEM_ON ) {
      BOOST_OFF;
      SWITCH_OFF;
      STATE_ON;
      DISCHG_ON; 
      system_power = SYSTEM_ON;
  } 
}
void Power_OFF(void)
{
  if( system_power != SYSTEM_OFF ) {
      BOOST_ON;
      SWITCH_ON;
      STATE_OFF;
      system_power = SYSTEM_OFF;
  }
}

void Power_Shundown(void)
{
  DISCHG_OFF;
  BOOST_OFF;
  SWITCH_OFF;  
}
void Adc_Init( void )
{
  ADC1_DeInit();
  ADC1_Init( ADC1_CONVERSIONMODE_SINGLE, \
             ADC1_CHANNEL_3, \
             ADC1_PRESSEL_FCPU_D2, \
             ADC1_EXTTRIG_TIM, \
             DISABLE, \
             ADC1_ALIGN_RIGHT, \
             ADC1_SCHMITTTRIG_CHANNEL3, \
             ENABLE );
}
void Adc_Get( int16_t *p_value )
{
  ADC1_StartConversion();
  while(!ADC1_GetFlagStatus(ADC1_FLAG_EOC));
  ADC1_ClearFlag(ADC1_FLAG_EOC);
  *p_value= ADC1_GetConversionValue();
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param file: pointer to the source file name
  * @param line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif