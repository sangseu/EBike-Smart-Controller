/*
 * EBike Smart Controller
 *
 * Copyright (C) Jorge Pinto aka Casainho, 2012.
 *
 *   casainho [at] gmail [dot] com
 *     www.casainho.net
 *
 * Released under the GPL License, Version 3
 */

#include "lpc210x.h"
#include "config.h"
#include "pwm.h"
#include "ios.h"
#include "timers.h"
#include "motor.h"
#include "adc.h"

#define PHASE_A 7 // green
#define PHASE_B 9 // yellow
#define PHASE_C 8 // blue

#define HALL_SENSORS_MASK ((1<<6) | (1<<4) | (1<<2)) // P0.2, P0.4, P0.6

void phase_a_h_on (void)
{
  /* LPC2103 P0.7 --> CPU1 */
  /* set to output */
  IODIR |= (1 << PHASE_A);
  IOSET = (1 << PHASE_A);
}

void phase_a_h_off (void)
{
  /* LPC2103 P0.7 --> CPU1 */
  /* set to output */
  IODIR |= (1 << PHASE_A);
  IOCLR = (1 << PHASE_A);
}

void phase_a_l_pwm_on (void)
{
  /* LPC2103 P0.12 (PWM; MAT1.2) --> CPU44 */
  PINSEL0 |= (1 << 25);
}

void phase_a_l_pwm_off (void)
{
  /* set to output */
  IODIR |= (1 << 12);
  IOSET = (1 << 12); // inverted logic

  /* LPC2103 P0.12 (PWM; MAT1.2) --> CPU44 */
  PINSEL0 &= ~(1 << 25);
}

void phase_b_h_on (void)
{
  /* LPC2103 P0.9 --> CPU5 */
  /* set to output */
  IODIR |= (1 << PHASE_B);
  IOSET = (1 << PHASE_B);
}

void phase_b_h_off (void)
{
  /* LPC2103 P0.9 --> CPU5 */
  /* set to output */
  IODIR |= (1 << PHASE_B);
  IOCLR = (1 << PHASE_B);
}

void phase_b_l_pwm_on (void)
{
  /* LPC2103 P0.19 (PWM; MAT1.0) --> CPU4 */
  PINSEL1 |= (1 << 7);
}

void phase_b_l_pwm_off (void)
{
  /* set to output */
  IODIR |= (1 << 19);
  IOSET = (1 << 19); // inverted logic

  /* LPC2103 P0.19 (PWM; MAT1.0) --> CPU4 */
  PINSEL1 &= ~(1 << 7);
}

void phase_c_h_on (void)
{
  /* LPC2103 P0.8 --> CPU3 */
  /* set to output */
  IODIR |= (1 << PHASE_C);
  IOSET = (1 << PHASE_C);
}

void phase_c_h_off (void)
{
  /* LPC2103 P0.8 --> CPU3 */
  /* set to output */
  IODIR |= (1 << PHASE_C);
  IOCLR = (1 << PHASE_C);
}

void phase_c_l_pwm_on (void)
{
  /* LPC2103 P0.13 (PWM; MAT1.1) --> CPU2 */
  PINSEL0 |= (1 << 27);
}

void phase_c_l_pwm_off (void)
{
  /* set to output */
  IODIR |= (1 << 13);
  IOSET = (1 << 13); // inverted logic

  /* LPC2103 P0.13 (PWM; MAT1.1) --> CPU2 */
  PINSEL0 &= ~(1 << 27);
}

void commutation_sector_1 (void)
{
  phase_a_h_off ();
  phase_a_l_pwm_on ();

  phase_b_h_off ();
  phase_b_l_pwm_off ();

  phase_c_l_pwm_off ();
  phase_c_h_on ();
}

void commutation_sector_2 (void)
{
  phase_a_h_off ();
  phase_a_l_pwm_off ();

  phase_b_h_off ();
  phase_b_l_pwm_on ();

  phase_c_l_pwm_off ();
  phase_c_h_on ();
}

void commutation_sector_3 (void)
{
  phase_a_l_pwm_off ();
  phase_a_h_on ();

  phase_b_h_off ();
  phase_b_l_pwm_on ();

  phase_c_h_off ();
  phase_c_l_pwm_off ();
}

void commutation_sector_4 (void)
{
  phase_a_l_pwm_off ();
  phase_a_h_on ();

  phase_b_h_off ();
  phase_b_l_pwm_off ();

  phase_c_h_off ();
  phase_c_l_pwm_on ();
}

void commutation_sector_5 (void)
{
  phase_a_h_off ();
  phase_a_l_pwm_off ();

  phase_b_l_pwm_off ();
  phase_b_h_on ();

  phase_c_h_off ();
  phase_c_l_pwm_on ();
}

void commutation_sector_6 (void)
{
  phase_a_h_off ();
  phase_a_l_pwm_on ();

  phase_b_l_pwm_off ();
  phase_b_h_on ();

  phase_c_h_off ();
  phase_c_l_pwm_off ();
}

void commutation_disable (void)
{
  phase_a_h_off ();
  phase_a_l_pwm_off ();

  phase_b_h_off ();
  phase_b_l_pwm_off ();

  phase_c_h_off ();
  phase_c_l_pwm_off ();
}

unsigned int get_current_sector (void)
{
  static unsigned int hall_sensors = 0;
  unsigned int i;

  // motor seems to run perfectly but not everytime is able to start and I can start by hand this times
  static unsigned int table[6] =
  {
        //  c b a
    80, //  1010000 == 80
    64, //  1000000 == 64
    68, //  1000100 == 68
     4, //  0000100 == 4
    20, //  0010100 == 20
    16  //  0010000 == 16
  };

  hall_sensors = (IOPIN & HALL_SENSORS_MASK); // mask other pins

  // go trough the table to identify the indice and calc the sector number
  for (i = 0; i < 6; i++)
  {
    if (table[i] == hall_sensors)
    {
      return (i + 1); // return the sector number
    }
  }

  return 0;
}

void commutate (void)
{
  volatile unsigned int sector;

  sector = get_current_sector ();

  switch (sector)
  {
    case 1:
    commutation_sector_1 ();
    break;

    case 2:
    commutation_sector_2 ();
    break;

    case 3:
    commutation_sector_3 ();
    break;

    case 4:
    commutation_sector_4 ();
    break;

    case 5:
    commutation_sector_5 ();
    break;

    case 6:
    commutation_sector_6 ();
    break;

    default:
    commutation_disable ();
    break;
  }
}

void commutation_sector (unsigned int sector)
{
  switch (sector)
  {
    case 1:
    commutation_sector_1 ();
    break;

    case 2:
    commutation_sector_2 ();
    break;

    case 3:
    commutation_sector_3 ();
    break;

    case 4:
    commutation_sector_4 ();
    break;

    case 5:
    commutation_sector_5 ();
    break;

    case 6:
    commutation_sector_6 ();
    break;

    default:
    commutation_disable ();
    break;
  }
}

unsigned int increment_sector (unsigned int sector)
{
  if (sector < 5)
  {
    sector++;
  }
  else // sector = 6
  {
    sector = 1;
  }

  return sector;
}

unsigned int decrement_sector (unsigned int sector)
{
  if (sector > 1)
  {
    sector--;
  }
  else // sector = 1
  {
    sector = 6;
  }

  return sector;
}

float delay_with_current_control (unsigned int us10, float current_max)
{
  unsigned int duty_cycle = 0;
  float current = 0;

  // 100ms align time
  micros10_clear (); // micros () = 0
  while (micros10() < us10) // while delay time, do...
  {
    current = motor_get_current ();
    if (current < current_max) // if currente is lower than max current
    {
      if (duty_cycle < 800) // limit here the duty_cycle
      {
        duty_cycle += 1;
      }
    }
    else if (current > current_max)
    {
      if (duty_cycle > 10)
      {
        duty_cycle -= 10;
      }
      else if (duty_cycle > 5)
      {
        duty_cycle -= 5;
      }
      else if (duty_cycle > 1)
      {
        duty_cycle -= 1;
      }
    }

    motor_set_duty_cycle (duty_cycle);
  }

  return current;
}

void phase_a_full (void)
{
  phase_a_h_on ();
  phase_a_l_pwm_off ();

  phase_b_h_off ();
  phase_b_l_pwm_on ();

  phase_c_h_off ();
  phase_c_l_pwm_on ();
}

void phase_b_full (void)
{
  phase_b_h_on ();
  phase_b_l_pwm_off ();

  phase_c_h_off ();
  phase_c_l_pwm_on ();

  phase_c_h_off ();
  phase_c_l_pwm_on ();
}

void phase_c_full (void)
{
  phase_c_h_on ();
  phase_c_l_pwm_off ();

  phase_b_h_off ();
  phase_b_l_pwm_on ();

  phase_a_h_off ();
  phase_a_l_pwm_on ();
}

void bldc_align (void)
{
  /* disable all mosfets */
  commutation_disable ();

  /* First, enable phase b and c low */
  // phase_b_l_on
  // set to output
  IODIR |= (1 << 19);
  IOCLR = (1 << 19); // inverted logic
  // LPC2103 P0.19 (PWM; MAT1.0) --> CPU4
  PINSEL1 &= ~(1 << 7);

  // phase_c_l_on
  // set to output
  IODIR |= (1 << 13);
  IOCLR = (1 << 13); // inverted logic
  // LPC2103 P0.13 (PWM; MAT1.1) --> CPU2
  PINSEL0 &= ~(1 << 27);

  /* Second, enable phase a for 100ms but control the current! */
  float current = 0;
  float current_max = 4;

  // 100ms align time
  micros10_clear (); // micros () = 0
  while (micros10() < 65000) // while delay time, do...
  {
    current = motor_get_current ();
    if (current < current_max) // if current is lower than max current
    {
      /* enable phase a high */
      phase_a_h_on ();
    }
    else if (current > current_max)
    {
      /* disable phase a high */
      phase_a_h_off ();
    }
  }

  // Align is done, disable all mosfets
  commutation_disable ();

  /*****************************************/

  /* First, enable phase a and c low */
  // phase_a_l_on
  /* set to output */
  IODIR |= (1 << 12);
  IOCLR = (1 << 12); // inverted logic
  /* LPC2103 P0.12 (PWM; MAT1.2) --> CPU44 */
  PINSEL0 &= ~(1 << 25);

  // phase_c_l_on
  // set to output
  IODIR |= (1 << 13);
  IOCLR = (1 << 13); // inverted logic
  // LPC2103 P0.13 (PWM; MAT1.1) --> CPU2
  PINSEL0 &= ~(1 << 27);

  // 100ms align time
  micros10_clear (); // micros () = 0
  while (micros10() < 65000) // while delay time, do...
  {
    current = motor_get_current ();
    if (current < current_max) // if current is lower than max current
    {
      /* enable phase a high */
      phase_b_h_on ();
    }
    else if (current > current_max)
    {
      /* disable phase a high */
      phase_b_h_off ();
    }
  }

  // Align is done, disable all mosfets
  commutation_disable ();


  /*****************************************/

  /* First, enable phase a and b low */
  // phase_a_l_on
  /* set to output */
  IODIR |= (1 << 12);
  IOCLR = (1 << 12); // inverted logic
  /* LPC2103 P0.12 (PWM; MAT1.2) --> CPU44 */
  PINSEL0 &= ~(1 << 25);

  // phase_b_l_on
  // set to output
  /* set to output */
  IODIR |= (1 << 19);
  IOCLR = (1 << 19); // inverted logic
  /* LPC2103 P0.19 (PWM; MAT1.0) --> CPU4 */
  PINSEL1 &= ~(1 << 7);

  // 100ms align time
  micros10_clear (); // micros () = 0
  while (micros10() < 65000) // while delay time, do...
  {
    current = motor_get_current ();
    if (current < current_max) // if current is lower than max current
    {
      /* enable phase a high */
      phase_c_h_on ();
    }
    else if (current > current_max)
    {
      /* disable phase a high */
      phase_c_h_off ();
    }
  }

  // Align is done, disable all mosfets
  commutation_disable ();

}