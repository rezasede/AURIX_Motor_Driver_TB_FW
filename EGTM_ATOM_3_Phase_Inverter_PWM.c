#include "EGTM_ATOM_3_Phase_Inverter_PWM.h"
#include "Ifx_Types.h"
#include "IfxEgtm_Pwm.h"
#include "IfxPort.h"
#include "IfxPort_Pinmap.h"
#include "math.h"

/* Configuration Parameters */
#define PWM_FREQ_HZ         20000.0f
#define START_FREQ_HZ       0.1f           // Starting frequency
#define END_FREQ_HZ         10.0f          // Target highest frequency
#define SWEEP_TIME_SEC      100.0f           // Time to reach target in seconds
#define MAX_DUTY_TICKS      100           // Replace with your timer's 100% period

/* Math Constants */
#define TWO_PI              6.28318530718f
#define PHASE_SHIFT_120     2.09439510239f

/* Pre-calculated Sweep Parameters to save CPU cycles in the ISR */
// Constant used to convert frequency to radians per tick
#define ANGLE_STEP_COEFF    (TWO_PI / PWM_FREQ_HZ)

// How much the frequency increases per 20kHz PWM tick
#define FREQ_STEP_PER_TICK  ((END_FREQ_HZ - START_FREQ_HZ) / (SWEEP_TIME_SEC * PWM_FREQ_HZ))

typedef struct
{
    IfxEgtm_Pwm pwm;
    IfxEgtm_Pwm_Channel channels[NUM_OF_CHANNELS];
    float32 dutyCycles[NUM_OF_CHANNELS];
    float32 phases[NUM_OF_CHANNELS];
    IfxEgtm_Pwm_DeadTime deadTimes[NUM_OF_CHANNELS];
} EgtmAtom3phInv;

IFX_STATIC EgtmAtom3phInv g_egtmAtom3phInv;

IFX_INTERRUPT(interruptEgtmAtom, 0, ISR_PRIORITY_ATOM);

void interruptEgtmAtom (void)
{
    // Static variables retain their state across ISR calls
    static float phase_angle = 0.0f;
    static float current_freq = START_FREQ_HZ;

    // 1. Linear Frequency Sweep (Ramp up)
    if (current_freq < END_FREQ_HZ)
    {
        current_freq += FREQ_STEP_PER_TICK;

        // Clamp to exact target to avoid floating point overshoot
        if (current_freq > END_FREQ_HZ)
        {
            current_freq = END_FREQ_HZ;
        }
    }

    // 2. Calculate the dynamic angle step for this specific tick
    float angle_step = current_freq * ANGLE_STEP_COEFF;

    // 3. Advance the phase angle
    phase_angle += angle_step;
    if (phase_angle >= TWO_PI)
    {
        phase_angle -= TWO_PI;
    }

    // 4. Calculate the 3-phase sine values (-1.0 to 1.0)
    float sine_R = sinf(phase_angle);
    float sine_S = sinf(phase_angle - PHASE_SHIFT_120);
    float sine_W = sinf(phase_angle + PHASE_SHIFT_120);

    // 5. Scale to Duty Cycle (0% to 95% limit preserved for bootstrap charging)
    uint32 duty_R = (uint32)((sine_R + 1.0f) * 0.475f * MAX_DUTY_TICKS);
    uint32 duty_S = (uint32)((sine_S + 1.0f) * 0.475f * MAX_DUTY_TICKS);
    uint32 duty_W = (uint32)((sine_W + 1.0f) * 0.475f * MAX_DUTY_TICKS);

    // 6. Send to the PWM update function
    updateEgtmAtom3phInvDuty(duty_R, duty_S, duty_W);

    /* Clear your interrupt flags here if necessary */
}


void IfxEgtm_periodEventFunction (void *data)
{

}

void initEgtmAtom3phInv (void)
{
    IfxEgtm_Pwm_Config config;
    IfxEgtm_Pwm_ChannelConfig channelConfig[NUM_OF_CHANNELS];
    IfxEgtm_Pwm_DtmConfig dtmConfig[NUM_OF_CHANNELS];
    IfxEgtm_Pwm_InterruptConfig interruptConfig;
    IfxEgtm_Pwm_OutputConfig output[NUM_OF_CHANNELS];

    IfxEgtm_Pwm_initConfig(&config, &MODULE_EGTM);

    output[0].pin = (IfxEgtm_Pwm_ToutMap*) PHASE_U_HS;
    output[0].complementaryPin = (IfxEgtm_Pwm_ToutMap*) PHASE_U_LS;
    output[0].polarity = Ifx_ActiveState_high;
    output[0].complementaryPolarity = Ifx_ActiveState_low;
    output[0].outputMode = IfxPort_OutputMode_pushPull;
    output[0].padDriver = IfxPort_PadDriver_cmosAutomotiveSpeed1;

    output[1].pin = (IfxEgtm_Pwm_ToutMap*) PHASE_V_HS;
    output[1].complementaryPin = (IfxEgtm_Pwm_ToutMap*) PHASE_V_LS;
    output[1].polarity = Ifx_ActiveState_high;
    output[1].complementaryPolarity = Ifx_ActiveState_low;
    output[1].outputMode = IfxPort_OutputMode_pushPull;
    output[1].padDriver = IfxPort_PadDriver_cmosAutomotiveSpeed1;

    output[2].pin = (IfxEgtm_Pwm_ToutMap*) PHASE_W_HS;
    output[2].complementaryPin = (IfxEgtm_Pwm_ToutMap*) PHASE_W_LS;
    output[2].polarity = Ifx_ActiveState_high;
    output[2].complementaryPolarity = Ifx_ActiveState_low;
    output[2].outputMode = IfxPort_OutputMode_pushPull;
    output[2].padDriver = IfxPort_PadDriver_cmosAutomotiveSpeed1;

    dtmConfig[0].deadTime.rising = 75e-9f;
    dtmConfig[0].deadTime.falling = 75e-9f;
    dtmConfig[1].deadTime.rising = 75e-9f;
    dtmConfig[1].deadTime.falling = 75e-9f;
    dtmConfig[2].deadTime.rising = 75e-9f;
    dtmConfig[2].deadTime.falling = 75e-9f;

    interruptConfig.mode = IfxEgtm_IrqMode_pulseNotify;
    interruptConfig.isrProvider = IfxSrc_Tos_cpu0;
    interruptConfig.priority = ISR_PRIORITY_ATOM;
    interruptConfig.vmId = IfxSrc_VmId_0;
    interruptConfig.periodEvent = IfxEgtm_periodEventFunction;
    interruptConfig.dutyEvent = NULL_PTR;

    channelConfig[0].timerCh = IfxEgtm_Pwm_SubModule_Ch_0;
    channelConfig[0].phase = 0.0f;
    channelConfig[0].duty = PHASE_U_DUTY;
    channelConfig[0].dtm = &dtmConfig[0];
    channelConfig[0].output = &output[0];
    channelConfig[0].mscOut = NULL_PTR;
    channelConfig[0].interrupt = &interruptConfig;

    channelConfig[1].timerCh = IfxEgtm_Pwm_SubModule_Ch_1;
    channelConfig[1].phase = 0.0f;
    channelConfig[1].duty = PHASE_V_DUTY;
    channelConfig[1].dtm = &dtmConfig[0];
    channelConfig[1].output = &output[1];
    channelConfig[1].mscOut = NULL_PTR;
    channelConfig[1].interrupt = NULL_PTR;

    channelConfig[2].timerCh = IfxEgtm_Pwm_SubModule_Ch_2;
    channelConfig[2].phase = 0.0f;
    channelConfig[2].duty = PHASE_W_DUTY;
    channelConfig[2].dtm = &dtmConfig[0];
    channelConfig[2].output = &output[2];
    channelConfig[2].mscOut = NULL_PTR;
    channelConfig[2].interrupt = NULL_PTR;

    config.cluster = IfxEgtm_Cluster_0;
    config.subModule = IfxEgtm_Pwm_SubModule_atom;
    config.alignment = IfxEgtm_Pwm_Alignment_center;
    config.syncStart = FALSE;
    config.numChannels = NUM_OF_CHANNELS;
    config.channels = channelConfig;
    config.frequency = PWM_FREQUENCY;
    config.clockSource.atom = IfxEgtm_Cmu_Clk_0;
    config.dtmClockSource = IfxEgtm_Dtm_ClockSource_cmuClock0;
    config.syncUpdateEnabled = TRUE;

    if (!IfxEgtm_isEnabled(&MODULE_EGTM))
    {
        float32 frequency;
        IfxEgtm_enable(&MODULE_EGTM);
        frequency = IfxEgtm_Cmu_getModuleFrequency(&MODULE_EGTM);
        IfxEgtm_Cmu_setGclkFrequency(&MODULE_EGTM, frequency);
        IfxEgtm_Cmu_setClkFrequency(&MODULE_EGTM, IfxEgtm_Cmu_Clk_0, frequency);
        IfxEgtm_Cmu_enableClocks(&MODULE_EGTM, IFXEGTM_CMU_CLKEN_CLK0);
    }

    /* Init Ch0-Ch2 via iLLD */
    IfxEgtm_Pwm_init(&g_egtmAtom3phInv.pwm, &g_egtmAtom3phInv.channels[0], &config);

    /*=== Manual Ch3: center-aligned trigger channel ===
     * Same counter mode as Ch0, synced by trigger chain.
     * SL=1 + tiny CM1 → output is HIGH almost everywhere,
     * with a brief LOW dip near valley, then rising edge AT the valley.
     * This rising edge triggers the ADC — duty-independent.
     */
    {
        Ifx_EGTM_CLS_ATOM *atom = &MODULE_EGTM.CLS[0].ATOM;
        uint32 periodTicks = atom->CH[0].CM0.U;
        uint32 settlingTicks = 1; /* ticks — sensor + ADC settling margin */ // 100 would be right at the momment when 1.2us pwm Width used
        uint32 tinyDuty = periodTicks / 1000;
        if (tinyDuty < 2)
            tinyDuty = 2;

        Ifx_EGTM_CLS_ATOM_CH_CTRL ctrl;
        ctrl.U = atom->CH[0].CTRL.U; /* Copy Ch0's working config */
        ctrl.B.RST_CCU0 = 0; /* Reset on trigger from chain (synced to Ch0) */
        ctrl.B.SL = 1; /* HIGH default → rising edge at CN0=CM1 near valley */
        ctrl.B.TRIGOUT = 0; /* Forward (not generate) */

        atom->CH[3].CTRL.U = ctrl.U;
        atom->CH[3].CTRL_SR.U = ctrl.U & 0xF800u;
        atom->CH[3].CM0.U = periodTicks;
        atom->CH[3].CM1.U = periodTicks - settlingTicks;
        atom->CH[3].SR0.U = periodTicks;
        atom->CH[3].SR1.U = periodTicks - settlingTicks;
        atom->CH[3].CN0.U = 0;
    }

    g_egtmAtom3phInv.dutyCycles[0] = channelConfig[0].duty;
    g_egtmAtom3phInv.dutyCycles[1] = channelConfig[1].duty;
    g_egtmAtom3phInv.dutyCycles[2] = channelConfig[2].duty;
    g_egtmAtom3phInv.deadTimes[0] = channelConfig[0].dtm->deadTime;
    g_egtmAtom3phInv.deadTimes[1] = channelConfig[1].dtm->deadTime;
    g_egtmAtom3phInv.deadTimes[2] = channelConfig[2].dtm->deadTime;
}

void startPWMinEGTM (void)
{
    Ifx_EGTM_CLS_ATOM *atom = &MODULE_EGTM.CLS[0].ATOM;

    /* Start Ch0-Ch2 via iLLD (this overwrites AGC) */
    IfxEgtm_Pwm_startSyncedChannels(&g_egtmAtom3phInv.pwm);

    /* Enable Ch3 via status registers (immediate, bypasses AGC update) */
    atom->AGC.ENDIS_STAT.B.ENDIS_STAT3 = 2;
    atom->AGC.OUTEN_STAT.B.OUTEN_STAT3 = 2;

    /* DTM pass-through for Ch3: active AND shadow registers */
    MODULE_EGTM.CLS[0].CDTM.DTM[0].CH_CTRL2.B.OC0_3 = 1;
    MODULE_EGTM.CLS[0].CDTM.DTM[0].CH_CTRL2_SR.B.OC0_3_SR = 1;
}

/* This function update duty cycles */
void updateEgtmAtom3phInvDuty (uint32 Duty_R, uint32 Duty_S, uint32 Duty_W)
{
    g_egtmAtom3phInv.dutyCycles[0] = Duty_R;
    g_egtmAtom3phInv.dutyCycles[1] = Duty_S;
    g_egtmAtom3phInv.dutyCycles[2] = Duty_W;

    /* Update duty of all configured channels to requested values immediately */
    IfxEgtm_Pwm_updateChannelsDutyImmediate(&g_egtmAtom3phInv.pwm, (float32*) g_egtmAtom3phInv.dutyCycles);
}

