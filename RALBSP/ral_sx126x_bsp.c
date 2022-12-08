/*********************************************************************
* COPYRIGHT 2022 CONNECTED DEVELOPMENT, A DIVISION OF EXPONENTIAL
* TECHNOLOGY GROUP.
*
* SPDX-License-Identifier: Apache-2.0
*
*******************************************************************************
* @file
* @brief  Connected Development implementation of the Radio Abstraction Layer
*         for the Connected Development SX1262 shield.
*
* @details  Copied from the example in the Semtech LoRa Basics Modem SDK at:
*             SWSD001\shields\SX126X\common\src\ral_sx126x_bsp.c
*             SWSD001\shields\SX126X\smtc_shield_sx126x\SX1262MB2CAS\src\smtc_shield_sx1262mb2cas.c
******************************************************************************/

/**
 * @file      ral_sx126x_bsp.c
 *
 * @brief     Board Support Package for the SX126x-specific Radio Abstraction Layer.
 *
 * Revised BSD License
 * Copyright Semtech Corporation 2022. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Semtech corporation nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL SEMTECH CORPORATION BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

#include "ral_sx126x_bsp.h"
#include "ralf_sx126x.h"
#include "sx126x_hal_context.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(RALBSP, CONFIG_LBM_LOG_LEVEL);

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS-----------------------------------------------------------
 */

#define LORA_RADIO_NODE_ID                   DT_ALIAS(lora0)
#define DIO2_TX_ENABLE                       DT_PROP(LORA_RADIO_NODE_ID, dio2_tx_enable)

#define CD_SHIELD_SX1262_SUBGHZ_FREQ_MIN     150000000
#define CD_SHIELD_SX1262_SUBGHZ_FREQ_MAX     960000000

#define CD_SHIELD_SX1262_MIN_PWR             -9
#define CD_SHIELD_SX1262_MAX_PWR             22


/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

/**
 * @brief Power amplifier and output power configurations structure definition.
 */
typedef struct CDShieldSx1262PaPwrCfg_s
{
   int8_t                    power;
   sx126x_pa_cfg_params_t    paConfig;
} CDShieldSx1262PaPwrCfg_t;


/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

// PA config table for the SX1262.
const CDShieldSx1262PaPwrCfg_t paCfgTable[CD_SHIELD_SX1262_MAX_PWR - CD_SHIELD_SX1262_MIN_PWR + 1] = {
   {   // Expected output power = -9dBm
      .power = 2,
      .paConfig = {
         .hp_max        = 0x01,
         .pa_duty_cycle = 0x01,
         .device_sel    = 0x00,
         .pa_lut        = 0x01,
      },
   },
   { // Expected output power = -8dBm
      .power = 5,
      .paConfig = {
         .hp_max        = 0x01,
         .pa_duty_cycle = 0x00,
         .device_sel    = 0x00,
         .pa_lut        = 0x01,
      },
   },
   { // Expected output power = -7dBm
      .power = 5,
      .paConfig = {
         .hp_max        = 0x01,
         .pa_duty_cycle = 0x01,
         .device_sel    = 0x00,
         .pa_lut        = 0x01,
      },
   },
   { // Expected output power = -6dBm
      .power = 8,
      .paConfig = {
         .hp_max        = 0x01,
         .pa_duty_cycle = 0x00,
         .device_sel    = 0x00,
         .pa_lut        = 0x01,
      },
   },
   { // Expected output power = -5dBm
      .power = 3,
      .paConfig = {
         .hp_max        = 0x02,
         .pa_duty_cycle = 0x00,
         .device_sel    = 0x00,
         .pa_lut        = 0x01,
      },
   },
   { // Expected output power = -4dBm
      .power = 9,
      .paConfig = {
         .hp_max        = 0x01,
         .pa_duty_cycle = 0x00,
         .device_sel    = 0x00,
         .pa_lut        = 0x01,
      },
   },
   { // Expected output power = -3dBm
      .power = 10,
      .paConfig = {
         .hp_max        = 0x01,
         .pa_duty_cycle = 0x00,
         .device_sel    = 0x00,
         .pa_lut        = 0x01,
      },
   },
   { // Expected output power = -2dBm
      .power = 11,
      .paConfig = {
         .hp_max        = 0x01,
         .pa_duty_cycle = 0x00,
         .device_sel    = 0x00,
         .pa_lut        = 0x01,
      },
   },
   { // Expected output power = -1dBm
      .power = 13,
      .paConfig = {
         .hp_max        = 0x01,
         .pa_duty_cycle = 0x01,
         .device_sel    = 0x00,
         .pa_lut        = 0x01,
      },
   },
   { // Expected output power = 0dBm
      .power = 19,
      .paConfig = {
         .hp_max        = 0x01,
         .pa_duty_cycle = 0x01,
         .device_sel    = 0x00,
         .pa_lut        = 0x01,
      },
   },
   { // Expected output power = 1dBm
      .power = 16,
      .paConfig = {
         .hp_max        = 0x01,
         .pa_duty_cycle = 0x01,
         .device_sel    = 0x00,
         .pa_lut        = 0x01,
      },
   },
   { // Expected output power = 2dBm
      .power = 20,
      .paConfig = {
         .hp_max        = 0x01,
         .pa_duty_cycle = 0x00,
         .device_sel    = 0x00,
         .pa_lut        = 0x01,
      },
   },
   { // Expected output power = 3dBm
      .power = 18,
      .paConfig = {
         .hp_max        = 0x01,
         .pa_duty_cycle = 0x03,
         .device_sel    = 0x00,
         .pa_lut        = 0x01,
      },
   },
   { // Expected output power = 4dBm
      .power = 21,
      .paConfig = {
         .hp_max        = 0x01,
         .pa_duty_cycle = 0x00,
         .device_sel    = 0x00,
         .pa_lut        = 0x01,
      },
   },
   { // Expected output power = 5dBm
      .power = 16,
      .paConfig = {
         .hp_max        = 0x02,
         .pa_duty_cycle = 0x00,
         .device_sel    = 0x00,
         .pa_lut        = 0x01,
      },
   },
   { // Expected output power = 6dBm
      .power = 22,
      .paConfig = {
         .hp_max        = 0x01,
         .pa_duty_cycle = 0x00,
         .device_sel    = 0x00,
         .pa_lut        = 0x01,
      },
   },
   { // Expected output power = 7dBm
      .power = 22,
      .paConfig = {
         .hp_max        = 0x01,
         .pa_duty_cycle = 0x01,
         .device_sel    = 0x00,
         .pa_lut        = 0x01,
      },
   },
   { // Expected output power = 8dBm
      .power = 22,
      .paConfig = {
         .hp_max        = 0x01,
         .pa_duty_cycle = 0x02,
         .device_sel    = 0x00,
         .pa_lut        = 0x01,
      },
   },
   { // Expected output power = 9dBm
      .power = 22,
      .paConfig = {
         .hp_max        = 0x01,
         .pa_duty_cycle = 0x03,
         .device_sel    = 0x00,
         .pa_lut        = 0x01,
      },
   },
   { // Expected output power = 10dBm
      .power = 22,
      .paConfig = {
         .hp_max        = 0x01,
         .pa_duty_cycle = 0x04,
         .device_sel    = 0x00,
         .pa_lut        = 0x01,
      },
   },
   { // Expected output power = 11dBm
      .power = 22,
      .paConfig = {
         .hp_max        = 0x02,
         .pa_duty_cycle = 0x00,
         .device_sel    = 0x00,
         .pa_lut        = 0x01,
      },
   },
   { // Expected output power = 12dBm
      .power = 22,
      .paConfig = {
         .hp_max        = 0x02,
         .pa_duty_cycle = 0x01,
         .device_sel    = 0x00,
         .pa_lut        = 0x01,
      },
   },
   { // Expected output power = 13dBm
      .power = 22,
      .paConfig = {
         .hp_max        = 0x02,
         .pa_duty_cycle = 0x02,
         .device_sel    = 0x00,
         .pa_lut        = 0x01,
      },
   },
   { // Expected output power = 14dBm
      .power = 22,
      .paConfig = {
         .hp_max        = 0x02,
         .pa_duty_cycle = 0x03,
         .device_sel    = 0x00,
         .pa_lut        = 0x01,
      },
   },
   { // Expected output power = 15dBm
      .power = 22,
      .paConfig = {
         .hp_max        = 0x03,
         .pa_duty_cycle = 0x01,
         .device_sel    = 0x00,
         .pa_lut        = 0x01,
      },
   },
   { // Expected output power = 16dBm
      .power = 22,
      .paConfig = {
         .hp_max        = 0x03,
         .pa_duty_cycle = 0x02,
         .device_sel    = 0x00,
         .pa_lut        = 0x01,
      },
   },
   { // Expected output power = 17dBm
      .power = 22,
      .paConfig = {
         .hp_max        = 0x05,
         .pa_duty_cycle = 0x00,
         .device_sel    = 0x00,
         .pa_lut        = 0x01,
      },
   },
   { // Expected output power = 18dBm
      .power = 22,
      .paConfig = {
         .hp_max        = 0x05,
         .pa_duty_cycle = 0x01,
         .device_sel    = 0x00,
         .pa_lut        = 0x01,
      },
   },
   { // Expected output power = 19dBm
      .power = 22,
      .paConfig = {
         .hp_max        = 0x05,
         .pa_duty_cycle = 0x02,
         .device_sel    = 0x00,
         .pa_lut        = 0x01,
      },
   },
   { // Expected output power = 20dBm
      .power = 22,
      .paConfig = {
         .hp_max        = 0x06,
         .pa_duty_cycle = 0x03,
         .device_sel    = 0x00,
         .pa_lut        = 0x01,
      },
   },
   { // Expected output power = 21dBm
      .power = 22,
      .paConfig = {
         .hp_max        = 0x06,
         .pa_duty_cycle = 0x04,
         .device_sel    = 0x00,
         .pa_lut        = 0x01,
      },
   },
   { // Expected output power = 22dBm
      .power = 22,
      .paConfig = {
         .hp_max        = 0x07,
         .pa_duty_cycle = 0x04,
         .device_sel    = 0x00,
         .pa_lut        = 0x01,
      },
   },
};


/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

// The context contains the SPI and GPIOs to the SX126X. The values are fetched from
// the LoRa shield overlay devicetree configuration.
static const sx126x_hal_context_t radioContext = {
   .spiSpec   = SPI_DT_SPEC_GET(LORA_RADIO_NODE_ID, SPI_WORD_SET(8) | SPI_TRANSFER_MSB, 0),
   .gpioCs    = SPI_CS_GPIOS_DT_SPEC_GET(LORA_RADIO_NODE_ID),
   .gpioReset = GPIO_DT_SPEC_GET(LORA_RADIO_NODE_ID, reset_gpios),
   .gpioBusy  = GPIO_DT_SPEC_GET(LORA_RADIO_NODE_ID, busy_gpios),
   .gpioDio1  = GPIO_DT_SPEC_GET(LORA_RADIO_NODE_ID, dio1_gpios),
};


/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */
static const CDShieldSx1262PaPwrCfg_t *CDShieldSx1262PaPwrCfg(const uint32_t rfFreqInHz,
                                                              const int8_t expectedOutputPwrInDbm);

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

/**
 * @brief Interface to initialise and return the ralf_t object corresponding to the
 *     SX126x shield.
 *
 * @warning It is up to the caller to ensure the context pointer stays valid for
 *    the life duration of the ralf_t created by this call.
 *
 * @returns Pointer to the ralf_t object of the board.
 */
ralf_t *smtc_board_initialise_and_get_ralf(void)
{
   static ralf_t localRalf = {0};

   localRalf = (ralf_t) RALF_SX126X_INSTANTIATE(&radioContext);
   return &localRalf;
}

/**
 * Get the regulator mode configuration.
 *
 * @param [in] context Chip implementation context.
 * @param [out] regMode Regulator mode of the SX1262 shield.
 */
void ral_sx126x_bsp_get_reg_mode(const void *context, sx126x_reg_mod_t *regMode)
{
   *regMode = SX126X_REG_MODE_DCDC;

   LOG_DBG("RegMode=%s", *regMode == SX126X_REG_MODE_DCDC ? "DC-DC" : "LDO");
}

/**
 * Get the internal RF switch configuration.
 *
 * @param [in] context Chip implementation context.
 * @param [out] dio2IsSetAsRfSwitch.
 */
void ral_sx126x_bsp_get_rf_switch_cfg(const void *context, bool *dio2IsSetAsRfSwitch)
{
   *dio2IsSetAsRfSwitch = DIO2_TX_ENABLE;

   LOG_DBG("DIO2 Tx Enable=%u", *dio2IsSetAsRfSwitch);
}

/**
 * Get the Tx-related configuration (power amplifier configuration, output power and ramp time) to be applied to the
 * chip.
 *
 * @param [in] context Chip implementation context.
 * @param [in] inputParams Parameters used to compute the chip configuration.
 * @param [out] outputParams Parameters to be configured in the chip.
 */
void ral_sx126x_bsp_get_tx_cfg(const void *context, const ral_sx126x_bsp_tx_cfg_input_params_t *inputParams,
                               ral_sx126x_bsp_tx_cfg_output_params_t *outputParams)
{
   const int8_t modemTxOffset = 0;

   const CDShieldSx1262PaPwrCfg_t *paPwrCfg = CDShieldSx1262PaPwrCfg(inputParams->freq_in_hz,
                                                                     inputParams->system_output_pwr_in_dbm + modemTxOffset);

   outputParams->chip_output_pwr_in_dbm_expected   = inputParams->system_output_pwr_in_dbm + modemTxOffset;
   outputParams->chip_output_pwr_in_dbm_configured = paPwrCfg->power;

   outputParams->pa_cfg.device_sel    = paPwrCfg->paConfig.device_sel;
   outputParams->pa_cfg.hp_max        = paPwrCfg->paConfig.hp_max;
   outputParams->pa_cfg.pa_duty_cycle = paPwrCfg->paConfig.pa_duty_cycle;
   outputParams->pa_cfg.pa_lut        = paPwrCfg->paConfig.pa_lut;
   outputParams->pa_ramp_time         = SX126X_RAMP_40_US;

   LOG_DBG("Frequency=%u ExpectedOutPwr=%d ConfiguredOutPower=%d PaDutyCycle=%u hpMax=%u",
           inputParams->freq_in_hz,
           outputParams->chip_output_pwr_in_dbm_expected,
           outputParams->chip_output_pwr_in_dbm_configured,
            outputParams->pa_cfg.pa_duty_cycle,
            outputParams->pa_cfg.hp_max);
}

/**
 * Get the XOSC configuration.
 *
 * @remark If no TCXO is present, this function should set tcxoIsRadioControlled to false, and return.
 *
 * @param [in] context Chip implementation context.
 * @param [out] tcxoIsRadioControlled Let the caller know if there is a radio-controlled TCXO.
 * @param [out] supplyVoltage TCXO supply voltage parameter.
 * @param [out] startupTimeInTick TCXO setup time in clock tick.
 */
void ral_sx126x_bsp_get_xosc_cfg(const void *context, bool *tcxoIsRadioControlled,
                                 sx126x_tcxo_ctrl_voltages_t *supplyVoltage, uint32_t *startupTimeInTick)
{
   *tcxoIsRadioControlled = false;
}

/**
 * Get the OCP (Over Current Protection) value.
 *
 * @param [in] context Chip implementation context.
 * @param [out] ocpInStepOf2p5Mma OCP value given in steps of 2.5 mA.
 */
void ral_sx126x_bsp_get_ocp_value(const void *context, uint8_t *ocpInStepOf2p5Mma)
{
   // From SX1261-2 Data Sheet, Table 5-2.
   *ocpInStepOf2p5Mma = 0x38;
}

/**
 * Get power amplifier and output power configuration for the given output power.
 *
 * @param [in] rfFreqInHz
 * @param [in] expectedOutputPwrInDbm
 *
 * @return Power amplifier and output power configuration.
 */
static const CDShieldSx1262PaPwrCfg_t *CDShieldSx1262PaPwrCfg(const uint32_t rfFreqInHz,
                                                              const int8_t expectedOutputPwrInDbm)
{
   if ((CD_SHIELD_SX1262_SUBGHZ_FREQ_MIN <= rfFreqInHz) && (rfFreqInHz <= CD_SHIELD_SX1262_SUBGHZ_FREQ_MAX))
   {
      uint32_t idx = expectedOutputPwrInDbm - CD_SHIELD_SX1262_MIN_PWR;
      if ((CD_SHIELD_SX1262_MIN_PWR <= expectedOutputPwrInDbm) &&
          (expectedOutputPwrInDbm <= CD_SHIELD_SX1262_MAX_PWR))
      {
         return &(paCfgTable[idx]);
      }
   }

   // If error, default to 6dBm.
   return &(paCfgTable[6 - CD_SHIELD_SX1262_MIN_PWR]);
}

