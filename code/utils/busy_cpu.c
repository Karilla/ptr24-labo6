/**
* \file busycpu.c
* \brief Fonctions permettant la simulation de consommation de temps CPU.
* \author Y. Thoma
* \version 0.3
* \date 08 novembre 2019
*
* Code Xenomai 3.0
*
*/

#include <alchemy/task.h>
#include <alchemy/alarm.h>
#include <alchemy/timer.h>

#include "busy_cpu.h"

void busy_cpu(int ns) {
  rt_timer_spin(ns);
}
