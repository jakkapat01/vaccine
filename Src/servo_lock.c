#include "app_constants.h"
#include "main.h"
#include "servo_lock.h"
#define SERVO_CLOSED_PULSE_US 1500U
#define SERVO_OPEN_PULSE_US 1000U
extern TIM_HandleTypeDef htim3;
static uint8_t active_command;

uint8_t Servo_Ready(void)
{
    return ((htim3.Instance == TIM3) && (htim3.State != HAL_TIM_STATE_RESET));
}

uint8_t Servo_Apply(uint8_t command)
{
    if (command == 0U)
    {
        return 1U;
    }
    else
    {
        /* No action. */
    }
    if ((command != 1U) && (command != 2U))
    {
        return 0U;
    }
    else
    {
        /* No action. */
    }
    if (command == active_command)
    {
        return 1U;
    }
    else
    {
        /* No action. */
    }
    if (command == 1U)
    {
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, SERVO_CLOSED_PULSE_US);
    }
    else
    {
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, SERVO_OPEN_PULSE_US);
    }
    /* Start only once; HAL marks the channel BUSY while PWM is running. */
    if ((active_command == 0U) && (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3) != HAL_OK))
    {
        return 0U;
    }
    else
    {
        /* No action. */
    }
    active_command = command;
    return 1U;
}
