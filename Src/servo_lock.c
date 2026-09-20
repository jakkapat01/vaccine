#include "main.h"
#include "servo_lock.h"
extern TIM_HandleTypeDef htim3;
static uint8_t active_command;
uint8_t Servo_Ready(void) {return htim3.Instance==TIM3 && htim3.State!=HAL_TIM_STATE_RESET;}
uint8_t Servo_Apply(uint8_t command) {
    if(!command) return 1;
    if(command!=1 && command!=2) return 0;
    if(command==active_command) return 1;
    __HAL_TIM_SET_COMPARE(&htim3,TIM_CHANNEL_3,command==1 ? 1500U : 1000U);
    /* Start only once; HAL marks the channel BUSY while PWM is running. */
    if(!active_command && HAL_TIM_PWM_Start(&htim3,TIM_CHANNEL_3)!=HAL_OK) return 0;
    active_command=command;return 1;
}
