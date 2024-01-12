/*******************************************************************************
 *  Copyright @ 2023
 *
 *  tuner&demod control extention
 *
 *
 *******************************************************************************/

/**
 * @brief    hw utils
 * @file    stbhwutils.c
 * @date    2023-12-19
 */

#include "stbhwtun.h"

#include "dtv_log.h"

#define TAG "UTILS"

U8BIT STB_Utils_StrengthToSSI(U8BIT path, S16BIT strength)
{
    int ssi = 0;

    switch (STB_TuneGetSignalType(path))
    {
        case TUNE_SIGNAL_COFDM:
            if ((STB_TuneGetActualTerrHpCodeRate(path) == TUNE_TCODERATE_2_3 &&
                STB_TuneGetActualTerrConstellation(path) == TUNE_TCONST_QAM256) ||
                (STB_TuneGetActualTerrHpCodeRate(path) == TUNE_TCODERATE_3_4 &&
                STB_TuneGetActualTerrConstellation(path) == TUNE_TCONST_QAM64))
            {
                if (strength <= -95)
                    ssi = 0;
                else if (strength <= -85)
                    ssi = 5 * (95 + strength) / 10;
                else if (strength <= -75)
                    ssi = 5 + 17 * (85 + strength) / 10;
                else if (strength <= -65)
                    ssi = 22 + 40 * (75 + strength) / 10;
                else if (strength <= -55)
                    ssi = 62 + 30 * (65 + strength) / 10;
                else if (strength <= -45)
                    ssi = 92 + 8 * (55 + strength) / 10;
                else
                    ssi = 100;
            }
            else if (STB_TuneGetActualTerrHpCodeRate(path) == TUNE_TCODERATE_3_4 &&
                STB_TuneGetActualTerrConstellation(path) == TUNE_TCONST_QAM256)
            {
                if (strength <= -95)
                    ssi = 0;
                else if (strength <= -85)
                    ssi = 3 * (95 + strength) / 10;
                else if (strength <= -75)
                    ssi = 3 + 11 * (85 + strength) / 10;
                else if (strength <= -65)
                    ssi = 14 + 40 * (75 + strength) / 10;
                else if (strength <= -55)
                    ssi = 54 + 35 * (65 + strength) / 10;
                else if (strength <= -45)
                    ssi = 89 + 9 * (55 + strength) / 10;
                else if (strength <= -40)
                    ssi = 98 + 2 * (45 + strength) / 5;
                else
                    ssi = 100;
            }
            else
            {
                if (strength <= -95)
                    ssi = 0;
                else if (strength <= -85)
                    ssi = 7 * (95 + strength) / 10;
                else if (strength <= -75)
                    ssi = 7 + 23 * (85 + strength) / 10;
                else if (strength <= -65)
                    ssi = 30 + 40 * (75 + strength) / 10;
                else if (strength <= -55)
                    ssi = 70 + 23 * (65 + strength) / 10;
                else if (strength <= -45)
                    ssi = 93 + 7 * (55 + strength) / 10;
                else
                    ssi = 100;
            }
            break;

        case TUNE_SIGNAL_QAM:
            if (STB_TuneGetActualCableMode(path) == TUNE_MODE_QAM_256)
            {
                if (strength <= -85)
                    ssi = 0;
                else if (strength <= -75)
                    ssi = 5 * (85 + strength) / 10;
                else if (strength <= -70)
                    ssi = 5 + 15 * (75 + strength) / 5;
                else if (strength <= -65)
                    ssi = 20 + 30 * (70 + strength) / 5;
                else if (strength <= -60)
                    ssi = 50 + 20 * (65 + strength) / 5;
                else if (strength <= -55)
                    ssi = 70 + 5 * (60 + strength) / 5;
                else if (strength <= -35)
                    ssi = 75 + 20 * (55 + strength) / 20;
                else if (strength <= -25)
                    ssi = 95 + 5 * (35 + strength) / 10;
                else
                    ssi = 100;
            }
            else if (STB_TuneGetActualCableMode(path) == TUNE_MODE_QAM_64)
            {
                if (strength <= -85)
                    ssi = 0;
                else if (strength <= -80)
                    ssi = 5 * (85 + strength) / 5;
                else if (strength <= -75)
                    ssi = 5 + 15 * (80 + strength) / 5;
                else if (strength <= -65)
                    ssi = 20 + 30 * (75 + strength) / 10;
                else if (strength <= -55)
                    ssi = 50 + 25 * (65 + strength) / 10;
                else if (strength <= -35)
                    ssi = 75 + 20 * (55 + strength) / 20;
                else if (strength <= -25)
                    ssi = 95 + 5 * (35 + strength) / 10;
                else
                    ssi = 100;
            }
            else//128Qam,as default
            {
                if (strength <= -85)
                    ssi = 0;
                else if (strength <= -75)
                    ssi = 10 * (85 + strength) / 10;
                else if (strength <= -65)
                    ssi = 10 + 40 * (75 + strength) / 10;
                else if (strength <= -60)
                    ssi = 50 + 20 * (65 + strength) / 5;
                else if (strength <= -55)
                    ssi = 70 + 5 * (60 + strength) / 5;
                else if (strength <= -35)
                    ssi = 75 + 20 * (55 + strength) / 20;
                else if (strength <= -25)
                    ssi = 95 + 5 * (35 + strength) / 10;
                else
                    ssi = 100;
            }
            break;

        case TUNE_SIGNAL_QPSK:
            if (strength <= -93)
                ssi = 0;
            else if (strength <= -90)
                ssi = 3 * (93 + strength) / 3;
            else if (strength <= -85)
                ssi = 3 + 7 * (90 + strength) / 5;
            else if (strength <= -75)
                ssi = 10 + 25 * (85 + strength) / 10;
            else if (strength <= -65)
                ssi = 35 + 45 * (75 + strength) / 10;
            else if (strength <= -55)
                ssi = 80 + 10 * (65 + strength) / 10;
            else if (strength <= -45)
                ssi = 90 + 8 * (55 + strength) / 10;
            else if (strength <= -35)
                ssi = 98 + 2 * (45 + strength) / 10;
            else
                ssi = 100;
            break;

        case TUNE_SIGNAL_ISDBT:

            if (strength <= -95)
                ssi = 0;
            else if (strength <= -85)
                ssi = 7 * (95 + strength) / 10;
            else if (strength <= -75)
                ssi = 7 + 23 * (85 + strength) / 10;
            else if (strength <= -65)
                ssi = 30 + 40 * (75 + strength) / 10;
            else if (strength <= -55)
                ssi = 70 + 23 * (65 + strength) / 10;
            else if (strength <= -45)
                ssi = 93 + 7 * (55 + strength) / 10;
            else
                ssi = 100;

            break;

        default:
            break;
    }

    return (U8BIT)ssi;
}

U8BIT STB_Utils_SNR10ToSQI(U8BIT path, S16BIT snr)
{
    int sqi = 0;

    switch (STB_TuneGetSignalType(path))
    {
        case TUNE_SIGNAL_COFDM:
            if (STB_TuneGetSystemType(path) == TUNE_SYSTEM_TYPE_DVBT2 &&
                STB_TuneGetActualTerrConstellation(path) == TUNE_TCONST_QAM256)
            {
                if (STB_TuneGetActualTerrHpCodeRate(path) == TUNE_TCODERATE_3_5)
                {
                    if (snr <= 165)
                        sqi = 0;
                    else if (snr <= 175)
                        sqi = 15 * (snr - 165) / 10;
                    else if (snr <= 185)
                        sqi = 15 + 20 * (snr - 175) / 10;
                    else if (snr <= 215)
                        sqi = 35 + 51 * (snr - 185) / 30;
                    else if (snr <= 225)
                        sqi = 86 + 10 * (snr - 215) / 10;
                    else if (snr <= 235)
                        sqi = 96 + 4 * (snr - 225) / 10;
                    else
                        sqi = 100;
                }
                else if (STB_TuneGetActualTerrHpCodeRate(path) == TUNE_TCODERATE_2_3)
                {
                    if (snr <= 175)
                        sqi = 0;
                    else if (snr <= 185)
                        sqi = 10 * (snr - 175) / 10;
                    else if (snr <= 195)
                        sqi = 10 + 20 * (snr - 185) / 10;
                    else if (snr <= 235)
                        sqi = 30 + 64 * (snr - 195) / 40;
                    else if (snr <= 245)
                        sqi = 94 + 6 * (snr - 235) / 10;
                    else
                        sqi = 100;
                }
                else//3/4,as default
                {
                    if (snr <= 195)
                        sqi = 0;
                    else if (snr <= 205)
                        sqi = 7 * (snr - 195) / 10;
                    else if (snr <= 215)
                        sqi = 7 + 17 * (snr - 205) / 10;
                    else if (snr <= 225)
                        sqi = 24 + 20 * (snr - 215) / 10;
                    else if (snr <= 255)
                        sqi = 44 + 48 * (snr - 225) / 30;
                    else if (snr <= 265)
                        sqi = 92 + 8 * (snr - 255) / 10;
                    else
                        sqi = 100;
                }
            }
            else if (STB_TuneGetActualTerrHpCodeRate(path) == TUNE_TCODERATE_3_4)
            {
                if (snr <= 165)
                    sqi = 0;
                else if (snr <= 175)
                    sqi = 9 * (snr - 165) / 10;
                else if (snr <= 195)
                    sqi = 9 + 32 * (snr - 175) / 20;
                else if (snr <= 215)
                    sqi = 41 + 40 * (snr - 195) / 20;
                else if (snr <= 225)
                    sqi = 81 + 12 * (snr - 215) / 10;
                else if (snr <= 235)
                    sqi = 93 + 7 * (snr - 225) / 10;
                else
                    sqi = 100;
            }
            else//2/3,as default
            {
                if (snr <= 145)
                    sqi = 0;
                else if (snr <= 155)
                    sqi = 8 * (snr - 145) / 10;
                else if (snr <= 165)
                    sqi = 8 + 14 * (snr - 155) / 10;
                else if (snr <= 195)
                    sqi = 22 + 54 * (snr - 165) / 30;
                else if (snr <= 215)
                    sqi = 76 + 20 * (snr - 195) / 20;
                else if (snr <= 225)
                    sqi = 96 + 4 * (snr - 215) / 10;
                else
                    sqi = 100;
            }
            break;

        case TUNE_SIGNAL_QAM:
            if (STB_TuneGetActualCableMode(path) == TUNE_MODE_QAM_256)
            {
                if (snr <= 265)
                    sqi = 0;
                else if (snr <= 270)
                    sqi = 15 * (snr - 265) / 5;
                else if (snr <= 275)
                    sqi = 15 + 10 * (snr - 265) / 5;
                else if (snr <= 315)
                    sqi = 25 + 40 * (snr - 275) / 40;
                else if (snr <= 325)
                    sqi = 65 + 25 * (snr - 315) / 10;
                else if (snr <= 335)
                    sqi = 90 + 10 * (snr - 325) / 10;
                else
                    sqi = 100;
            }
            else if (STB_TuneGetActualCableMode(path) == TUNE_MODE_QAM_64)
            {
                if (snr <= 205)
                    sqi = 0;
                else if (snr <= 215)
                    sqi = 15 * (snr - 205) / 10;
                else if (snr <= 225)
                    sqi = 15 + 20 * (snr - 215) / 10;
                else if (snr <= 245)
                    sqi = 35 + 30 * (snr - 225) / 20;
                else if (snr <= 255)
                    sqi = 65 + 10 * (snr - 245) / 10;
                else if (snr <= 265)
                    sqi = 75 + 15 * (snr - 255) / 10;
                else if (snr <= 275)
                    sqi = 90 + 10 * (snr - 265) / 10;
                else
                    sqi = 100;
            }
            else//16Qam,as default
            {
                if (snr <= 155)
                    sqi = 0;
                else if (snr <= 165)
                    sqi = 20 * (snr - 155) / 10;
                else if (snr <= 175)
                    sqi = 20 + 15 * (snr - 165) / 10;
                else if (snr <= 185)
                    sqi = 35 + 20 * (snr - 175) / 10;
                else if (snr <= 195)
                    sqi = 55 + 10 * (snr - 185) / 10;
                else if (snr <= 205)
                    sqi = 65 + 25 * (snr - 195) / 10;
                else if (snr <= 215)
                    sqi = 90 + 10 * (snr - 205) / 10;
                else
                    sqi = 100;
            }
            break;

        case TUNE_SIGNAL_QPSK:
            if (STB_TuneGetSystemType(path) == TUNE_SYSTEM_TYPE_DVBS2)
            {
                if (snr <= 75)
                    sqi = 0;
                else if (snr <= 85)
                    sqi = 15 * (snr - 75) / 10;
                else if (snr <= 105)
                    sqi = 15 + 20 * (snr - 85) / 20;
                else if (snr <= 115)
                    sqi = 35 + 17 * (snr - 105) / 10;
                else if (snr <= 125)
                    sqi = 52 + 27 * (snr - 115) / 10;
                else if (snr <= 135)
                    sqi = 79 + 6 * (snr - 125) / 10;
                else if (snr <= 145)
                    sqi = 85 + 15 * (snr - 135) / 10;
                else
                    sqi = 100;
            }
            else
            {
                if (snr <= 55)
                    sqi = 0;
                else if (snr <= 65)
                    sqi = 15 * (snr - 55) / 10;
                else if (snr <= 75)
                    sqi = 15 + 10 * (snr - 65) / 10;
                else if (snr <= 85)
                    sqi = 25 + 25 * (snr - 75) / 10;
                else if (snr <= 105)
                    sqi = 50 + 40 * (snr - 85) / 20;
                else if (snr <= 125)
                    sqi = 90 + 10 * (snr - 105) / 20;
                else
                    sqi = 100;
            }
            break;

        case TUNE_SIGNAL_ISDBT:

            if (snr <= 160)
                sqi = 0;
            else if (snr <= 170)
                sqi = 31 * (snr - 160) / 10;
            else if (snr <= 180)
                sqi = 31 + 18 * (snr - 170) / 10;
            else if (snr <= 190)
                sqi = 49 + 16 * (snr - 180) / 10;
            else if (snr <= 200)
                sqi = 65 + 15 * (snr - 190) / 10;
            else if (snr <= 210)
                sqi = 80 + 12 * (snr - 200) / 10;
            else if (snr <= 220)
                sqi = 92 + 8 * (snr - 210) / 10;
            else
                sqi = 100;

            break;

        default:
            break;
    }

    return (U8BIT)sqi;
}

