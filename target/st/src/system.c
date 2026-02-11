//
// Created by Süleyman ÇİÇEK <suleyman@cicek.pw> on 2/6/26.
//

#include "hal.h"

/**
  * @brief This function handles Non maskable interrupt.
  */
__attribute__((weak)) void NMI_Handler(void)
{
   while (1)
  {
  }
}

/**
  * @brief This function handles Hard fault interrupt.
  */
__attribute__((weak)) void HardFault_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles Memory management fault.
  */
__attribute__((weak)) void MemManage_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
__attribute__((weak)) void BusFault_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
__attribute__((weak)) void UsageFault_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
__attribute__((weak)) void SVC_Handler(void)
{

}

/**
  * @brief This function handles Debug monitor.
  */
__attribute__((weak)) void DebugMon_Handler(void)
{

}

/**
  * @brief This function handles Pendable request for system service.
  */
__attribute__((weak)) void PendSV_Handler(void)
{

}

/**
  * @brief This function handles System tick timer.
  */
__attribute__((weak)) void SysTick_Handler(void)
{
  HAL_IncTick();
}