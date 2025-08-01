#include "JCBTransient.h"

namespace JCBTransient {

/****************************************************************************************
Copyright (c) 2023 Cycling '74

The code that Max generates automatically and that end users are capable of
exporting and using, and any associated documentation files (the “Software”)
is a work of authorship for which Cycling '74 is the author and owner for
copyright purposes.

This Software is dual-licensed either under the terms of the Cycling '74
License for Max-Generated Code for Export, or alternatively under the terms
of the General Public License (GPL) Version 3. You may use the Software
according to either of these licenses as it is most appropriate for your
project on a case-by-case basis (proprietary or not).

A) Cycling '74 License for Max-Generated Code for Export

A license is hereby granted, free of charge, to any person obtaining a copy
of the Software (“Licensee”) to use, copy, modify, merge, publish, and
distribute copies of the Software, and to permit persons to whom the Software
is furnished to do so, subject to the following conditions:

The Software is licensed to Licensee for all uses that do not include the sale,
sublicensing, or commercial distribution of software that incorporates this
source code. This means that the Licensee is free to use this software for
educational, research, and prototyping purposes, to create musical or other
creative works with software that incorporates this source code, or any other
use that does not constitute selling software that makes use of this source
code. Commercial distribution also includes the packaging of free software with
other paid software, hardware, or software-provided commercial services.

For entities with UNDER 200k USD in annual revenue or funding, a license is hereby
granted, free of charge, for the sale, sublicensing, or commercial distribution
of software that incorporates this source code, for as long as the entity's
annual revenue remains below 200k USD annual revenue or funding.

For entities with OVER 200k USD in annual revenue or funding interested in the
sale, sublicensing, or commercial distribution of software that incorporates
this source code, please send inquiries to licensing (at) cycling74.com.

The above copyright notice and this license shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Please see
https://support.cycling74.com/hc/en-us/articles/360050779193-Gen-Code-Export-Licensing-FAQ
for additional information

B) General Public License Version 3 (GPLv3)
Details of the GPLv3 license can be found at: https://www.gnu.org/licenses/gpl-3.0.html
****************************************************************************************/

// global noise generator
Noise noise;
static const int GENLIB_LOOPCOUNT_BAIL = 100000;


// The State struct contains all the state and procedures for the gendsp kernel
typedef struct State {
	CommonState __commonstate;
	Delay m_rightInputDelay_12;
	Delay m_leftInputDelay_13;
	Delay m_sidechainTapDelay_11;
	Delay m_rightOutputDelay_14;
	Delay m_leftOutputDelay_15;
	Delay m_sidechainWriteDelay_10;
	int __exception;
	int vectorsize;
	t_sample m_makeupGainHistory_66;
	t_sample m_deltaModeHistory_67;
	t_sample m_soloSidechainHistory_65;
	t_sample m_bypassAmountHistory_64;
	t_sample m_smoothAmountHistory_58;
	t_sample m_holdCounterPlusEquals_68;
	t_sample m_sustainGainHistory_70;
	t_sample m_sensitivityHistory_69;
	t_sample m_diffHistory_72;
	t_sample m_attackGainHistory_71;
	t_sample m_lookaheadHistory_57;
	t_sample m_keyMixHistory_63;
	t_sample m_scEnableHistory_54;
	t_sample m_trimHistory_61;
	t_sample m_sidechainTrimHistory_60;
	t_sample m_rangeHistory_59;
	t_sample m_softclipHistory_53;
	t_sample m_dryWetMixHistory_62;
	t_sample m_lpfFreqHistory_55;
	t_sample m_hpfFreqHistory_56;
	t_sample m_diffHistory_73;
	t_sample m_transientSmoothHistory_74;
	t_sample m_n_LOOKAHEAD_78;
	t_sample m_p_BYPASS_76;
	t_sample m_i_MAKEUP_91;
	t_sample m_z_SMOOTH_90;
	t_sample m_j_HPF_89;
	t_sample m_j_HPFORDER_88;
	t_sample m_e_REL_92;
	t_sample m_d_ATK_94;
	t_sample m_f_HOLD_93;
	t_sample m_c_SUSTAIN_GAIN_96;
	t_sample m_q_SENSITIVITY_95;
	t_sample m_envelopeHistory_75;
	t_sample m_k_LPF_87;
	t_sample m_l_SC_85;
	t_sample m_h_DELTAMODE_79;
	t_sample m_atkHistory_52;
	t_sample m_u_SOFTCLIP_77;
	t_sample m_k_LPFORDER_86;
	t_sample m_v_DELTA_80;
	t_sample m_m_SOLOSC_82;
	t_sample m_o_DRYWET_81;
	t_sample m_y_SCTRIM_84;
	t_sample m_r_KEY_83;
	t_sample m_relHistory_51;
	t_sample m_lpfHistory_48;
	t_sample m_lpfHistory_49;
	t_sample m_hpfHistory_20;
	t_sample m_hpfHistory_19;
	t_sample m_hpfHistory_18;
	t_sample m_hpfOrderHistory_17;
	t_sample m_hpfHistory_21;
	t_sample m_hpfHistory_23;
	t_sample m_hpfHistory_22;
	t_sample m_hpfHistory_25;
	t_sample m_hpfHistory_24;
	t_sample m_lpfHistory_26;
	t_sample m_lpfOrderHistory_16;
	t_sample m_releaseHistSharp_8;
	t_sample m_rmsExpHistory_2;
	t_sample m_smoothFilterHistory_1;
	t_sample samplerate;
	t_sample m_peakEnvSharp_9;
	t_sample m_hold_counter_sharp_3;
	t_sample m_reductionHistRight_5;
	t_sample m_held_peak_sharp_4;
	t_sample m_attackHistSharp_7;
	t_sample m_reductionHistLeft_6;
	t_sample m_holdHistory_50;
	t_sample m_lpfHistory_27;
	t_sample m_lpfHistory_29;
	t_sample m_lpfHistory_43;
	t_sample m_lpfHistory_42;
	t_sample m_hpfHistory_41;
	t_sample m_hpfHistory_40;
	t_sample m_lpfHistory_44;
	t_sample m_lpfHistory_46;
	t_sample m_lpfHistory_45;
	t_sample m_b_ATTACK_GAIN_97;
	t_sample m_lpfHistory_47;
	t_sample m_lpfHistory_28;
	t_sample m_hpfHistory_39;
	t_sample m_hpfHistory_37;
	t_sample m_lpfHistory_32;
	t_sample m_lpfHistory_31;
	t_sample m_lpfHistory_30;
	t_sample m_hpfHistory_38;
	t_sample m_lpfHistory_33;
	t_sample m_hpfHistory_35;
	t_sample m_hpfHistory_34;
	t_sample m_hpfHistory_36;
	t_sample m_a_TRIM_98;
	// re-initialize all member variables;
	inline void reset(t_param __sr, int __vs) {
		__exception = 0;
		vectorsize = __vs;
		samplerate = __sr;
		m_smoothFilterHistory_1 = ((int)0);
		m_rmsExpHistory_2 = ((int)0);
		m_hold_counter_sharp_3 = ((int)0);
		m_held_peak_sharp_4 = ((int)0);
		m_reductionHistRight_5 = ((int)0);
		m_reductionHistLeft_6 = ((int)0);
		m_attackHistSharp_7 = ((int)0);
		m_releaseHistSharp_8 = ((int)0);
		m_peakEnvSharp_9 = ((int)0);
		m_sidechainWriteDelay_10.reset("m_sidechainWriteDelay_10", (((t_sample)0.025) * samplerate));
		m_sidechainTapDelay_11.reset("m_sidechainTapDelay_11", (((t_sample)0.025) * samplerate));
		m_rightInputDelay_12.reset("m_rightInputDelay_12", (((t_sample)0.025) * samplerate));
		m_leftInputDelay_13.reset("m_leftInputDelay_13", (((t_sample)0.025) * samplerate));
		m_rightOutputDelay_14.reset("m_rightOutputDelay_14", (((t_sample)0.025) * samplerate));
		m_leftOutputDelay_15.reset("m_leftOutputDelay_15", (((t_sample)0.025) * samplerate));
		m_lpfOrderHistory_16 = ((int)0);
		m_hpfOrderHistory_17 = ((int)0);
		m_hpfHistory_18 = ((int)0);
		m_hpfHistory_19 = ((int)0);
		m_hpfHistory_20 = ((int)0);
		m_hpfHistory_21 = ((int)0);
		m_hpfHistory_22 = ((int)0);
		m_hpfHistory_23 = ((int)0);
		m_hpfHistory_24 = ((int)0);
		m_hpfHistory_25 = ((int)0);
		m_lpfHistory_26 = ((int)0);
		m_lpfHistory_27 = ((int)0);
		m_lpfHistory_28 = ((int)0);
		m_lpfHistory_29 = ((int)0);
		m_lpfHistory_30 = ((int)0);
		m_lpfHistory_31 = ((int)0);
		m_lpfHistory_32 = ((int)0);
		m_lpfHistory_33 = ((int)0);
		m_hpfHistory_34 = ((int)0);
		m_hpfHistory_35 = ((int)0);
		m_hpfHistory_36 = ((int)0);
		m_hpfHistory_37 = ((int)0);
		m_hpfHistory_38 = ((int)0);
		m_hpfHistory_39 = ((int)0);
		m_hpfHistory_40 = ((int)0);
		m_hpfHistory_41 = ((int)0);
		m_lpfHistory_42 = ((int)0);
		m_lpfHistory_43 = ((int)0);
		m_lpfHistory_44 = ((int)0);
		m_lpfHistory_45 = ((int)0);
		m_lpfHistory_46 = ((int)0);
		m_lpfHistory_47 = ((int)0);
		m_lpfHistory_48 = ((int)0);
		m_lpfHistory_49 = ((int)0);
		m_holdHistory_50 = ((int)0);
		m_relHistory_51 = ((int)0);
		m_atkHistory_52 = ((int)0);
		m_softclipHistory_53 = ((int)0);
		m_scEnableHistory_54 = ((int)0);
		m_lpfFreqHistory_55 = ((int)0);
		m_hpfFreqHistory_56 = ((int)0);
		m_lookaheadHistory_57 = ((int)0);
		m_smoothAmountHistory_58 = ((int)0);
		m_rangeHistory_59 = ((int)0);
		m_sidechainTrimHistory_60 = ((int)0);
		m_trimHistory_61 = ((int)0);
		m_dryWetMixHistory_62 = ((int)0);
		m_keyMixHistory_63 = ((int)0);
		m_bypassAmountHistory_64 = ((int)0);
		m_soloSidechainHistory_65 = ((int)0);
		m_makeupGainHistory_66 = ((int)0);
		m_deltaModeHistory_67 = ((int)0);
		m_holdCounterPlusEquals_68 = ((int)0);
		m_sensitivityHistory_69 = ((int)0);
		m_sustainGainHistory_70 = ((int)0);
		m_attackGainHistory_71 = ((int)0);
		m_diffHistory_72 = ((int)0);
		m_diffHistory_73 = ((int)0);
		m_transientSmoothHistory_74 = ((int)0);
		m_envelopeHistory_75 = ((int)0);
		m_p_BYPASS_76 = 0;
		m_u_SOFTCLIP_77 = 0;
		m_n_LOOKAHEAD_78 = 0;
		m_h_DELTAMODE_79 = 0;
		m_v_DELTA_80 = 0;
		m_o_DRYWET_81 = 1;
		m_m_SOLOSC_82 = 0;
		m_r_KEY_83 = 0;
		m_y_SCTRIM_84 = 0;
		m_l_SC_85 = 0;
		m_k_LPFORDER_86 = 0;
		m_k_LPF_87 = 20000;
		m_j_HPFORDER_88 = 0;
		m_j_HPF_89 = 20;
		m_z_SMOOTH_90 = 0;
		m_i_MAKEUP_91 = 0;
		m_e_REL_92 = 60;
		m_f_HOLD_93 = 0;
		m_d_ATK_94 = 1;
		m_q_SENSITIVITY_95 = 0.5;
		m_c_SUSTAIN_GAIN_96 = 0;
		m_b_ATTACK_GAIN_97 = 0;
		m_a_TRIM_98 = 0;
		genlib_reset_complete(this);
		
	};
	// the signal processing routine;
	inline int perform(t_sample ** __ins, t_sample ** __outs, int __n) {
		vectorsize = __n;
		const t_sample * __in1 = __ins[0];
		const t_sample * __in2 = __ins[1];
		const t_sample * __in3 = __ins[2];
		const t_sample * __in4 = __ins[3];
		t_sample * __out1 = __outs[0];
		t_sample * __out2 = __outs[1];
		t_sample * __out3 = __outs[2];
		t_sample * __out4 = __outs[3];
		t_sample * __out5 = __outs[4];
		t_sample * __out6 = __outs[5];
		t_sample * __out7 = __outs[6];
		t_sample * __out8 = __outs[7];
		if (__exception) {
			return __exception;
			
		} else if (( (__in1 == 0) || (__in2 == 0) || (__in3 == 0) || (__in4 == 0) || (__out1 == 0) || (__out2 == 0) || (__out3 == 0) || (__out4 == 0) || (__out5 == 0) || (__out6 == 0) || (__out7 == 0) || (__out8 == 0) )) {
			__exception = GENLIB_ERR_NULL_BUFFER;
			return __exception;
			
		};
		t_sample bypassInverted = (((int)1) - m_p_BYPASS_76);
		t_sample smo_factor = (((int)1) + (m_z_SMOOTH_90 * ((int)4)));
		// the main sample loop;
		while ((__n--)) {
			const t_sample in1 = (*(__in1++));
			const t_sample in2 = (*(__in2++));
			const t_sample in3 = (*(__in3++));
			const t_sample in4 = (*(__in4++));
			t_sample smoothDeltaMode = ((m_deltaModeHistory_67 * ((t_sample)0.999)) + (m_v_DELTA_80 * ((t_sample)0.001)));
			t_sample deltaModeHistoryNext = fixdenorm(smoothDeltaMode);
			t_sample smoothAttackGain = ((m_attackGainHistory_71 * ((t_sample)0.999)) + (m_b_ATTACK_GAIN_97 * ((t_sample)0.001)));
			t_sample attackGainHistoryNext = fixdenorm(smoothAttackGain);
			t_sample smoothSustainGain = ((m_sustainGainHistory_70 * ((t_sample)0.999)) + (m_c_SUSTAIN_GAIN_96 * ((t_sample)0.001)));
			t_sample sustainGainHistoryNext = fixdenorm(smoothSustainGain);
			t_sample smoothSensitivity = ((m_sensitivityHistory_69 * ((t_sample)0.999)) + (m_q_SENSITIVITY_95 * ((t_sample)0.001)));
			t_sample sensitivityHistoryNext = fixdenorm(smoothSensitivity);
			t_sample smoothHpfOrder = ((m_hpfOrderHistory_17 * ((t_sample)0.999)) + (m_j_HPFORDER_88 * ((t_sample)0.001)));
			t_sample hpfOrderHistoryNext = fixdenorm(smoothHpfOrder);
			t_sample smoothLpfOrder = ((m_lpfOrderHistory_16 * ((t_sample)0.999)) + (m_k_LPFORDER_86 * ((t_sample)0.001)));
			t_sample lpfOrderHistoryNext = fixdenorm(smoothLpfOrder);
			int hpfOrderDiscrete = ((int)0);
			int lpfOrderDiscrete = ((int)0);
			if ((smoothHpfOrder < ((t_sample)0.5))) {
				hpfOrderDiscrete = ((int)0);
				
			} else {
				hpfOrderDiscrete = ((int)1);
				
			};
			if ((smoothLpfOrder < ((t_sample)0.5))) {
				lpfOrderDiscrete = ((int)0);
				
			} else {
				lpfOrderDiscrete = ((int)1);
				
			};
			t_sample smoothRange = ((m_rangeHistory_59 * ((t_sample)0.999)) + (m_h_DELTAMODE_79 * ((t_sample)0.001)));
			t_sample rangeHistoryNext = fixdenorm(smoothRange);
			t_sample smoothSmoothAmount = ((m_smoothAmountHistory_58 * ((t_sample)0.999)) + (m_z_SMOOTH_90 * ((t_sample)0.001)));
			t_sample smoothAmountHistoryNext = fixdenorm(smoothSmoothAmount);
			t_sample smoothAttack = ((m_atkHistory_52 * ((t_sample)0.999)) + (m_d_ATK_94 * ((t_sample)0.001)));
			t_sample atkHistoryNext = fixdenorm(smoothAttack);
			t_sample smoothRelease = ((m_relHistory_51 * ((t_sample)0.999)) + (m_e_REL_92 * ((t_sample)0.001)));
			t_sample relHistoryNext = fixdenorm(smoothRelease);
			t_sample smoothHold = ((m_holdHistory_50 * ((t_sample)0.999)) + (m_f_HOLD_93 * ((t_sample)0.001)));
			t_sample holdHistoryNext = fixdenorm(smoothHold);
			t_sample smoothHpfFreq = ((m_hpfFreqHistory_56 * ((t_sample)0.999)) + (m_j_HPF_89 * ((t_sample)0.001)));
			t_sample hpfFreqHistoryNext = fixdenorm(smoothHpfFreq);
			t_sample smoothLpfFreq = ((m_lpfFreqHistory_55 * ((t_sample)0.999)) + (m_k_LPF_87 * ((t_sample)0.001)));
			t_sample lpfFreqHistoryNext = fixdenorm(smoothLpfFreq);
			t_sample smoothScEnable = ((m_scEnableHistory_54 * ((t_sample)0.999)) + (m_l_SC_85 * ((t_sample)0.001)));
			t_sample scEnableHistoryNext = fixdenorm(smoothScEnable);
			t_sample smoothSoftclip = ((m_softclipHistory_53 * ((t_sample)0.999)) + (m_u_SOFTCLIP_77 * ((t_sample)0.001)));
			t_sample softclipHistoryNext = fixdenorm(smoothSoftclip);
			t_sample smoothLookahead = ((m_lookaheadHistory_57 * ((t_sample)0.9999)) + (m_n_LOOKAHEAD_78 * ((t_sample)9.9999999999989e-05)));
			t_sample lookaheadHistoryNext = fixdenorm(smoothLookahead);
			t_sample lookaheadSamples = (smoothLookahead * (samplerate * 0.001));
			t_sample leftDelayedForCompression = m_leftOutputDelay_15.read_step(lookaheadSamples);
			t_sample rightDelayedForCompression = m_rightOutputDelay_14.read_step(lookaheadSamples);
			t_sample leftDelayedForMixing = m_leftInputDelay_13.read_step(lookaheadSamples);
			t_sample rightDelayedForMixing = m_rightInputDelay_12.read_step(lookaheadSamples);
			t_sample smoothMakeupGain = ((m_makeupGainHistory_66 * ((t_sample)0.999)) + (m_i_MAKEUP_91 * ((t_sample)0.001)));
			t_sample makeupGainHistoryNext = fixdenorm(smoothMakeupGain);
			t_sample sidechainDelayedTap = m_sidechainTapDelay_11.read_step(lookaheadSamples);
			t_sample smoothSoloSidechain = ((m_soloSidechainHistory_65 * ((t_sample)0.999)) + (m_m_SOLOSC_82 * ((t_sample)0.001)));
			t_sample soloSidechainHistoryNext = fixdenorm(smoothSoloSidechain);
			t_sample sidechainDelayedWrite = m_sidechainWriteDelay_10.read_step(lookaheadSamples);
			t_sample smoothBypassAmount = ((m_bypassAmountHistory_64 * ((t_sample)0.999)) + (bypassInverted * ((t_sample)0.001)));
			t_sample bypassAmountHistoryNext = fixdenorm(smoothBypassAmount);
			t_sample smoothKeyMix = ((m_keyMixHistory_63 * ((t_sample)0.999)) + (m_r_KEY_83 * ((t_sample)0.001)));
			t_sample keyMixHistoryNext = fixdenorm(smoothKeyMix);
			t_sample smoothDryWetMix = ((m_dryWetMixHistory_62 * ((t_sample)0.999)) + (m_o_DRYWET_81 * ((t_sample)0.001)));
			t_sample dryWetMixHistoryNext = fixdenorm(smoothDryWetMix);
			t_sample mix_3814 = (smoothDryWetMix + (smoothDeltaMode * (((int)1) - smoothDryWetMix)));
			t_sample smoothTrimGain = ((m_trimHistory_61 * ((t_sample)0.999)) + (m_a_TRIM_98 * ((t_sample)0.001)));
			t_sample trimHistoryNext = fixdenorm(smoothTrimGain);
			t_sample trimGainLinear = dbtoa(smoothTrimGain);
			t_sample leftTrimmed = (in1 * trimGainLinear);
			t_sample rightTrimmed = (in2 * trimGainLinear);
			m_leftInputDelay_13.write(in1);
			m_rightInputDelay_12.write(in2);
			m_leftOutputDelay_15.write(leftTrimmed);
			m_rightOutputDelay_14.write(rightTrimmed);
			t_sample mainSignalMono = ((rightTrimmed + leftTrimmed) * ((t_sample)0.707));
			t_sample smoothSidechainTrimGain = ((m_sidechainTrimHistory_60 * ((t_sample)0.999)) + (m_y_SCTRIM_84 * ((t_sample)0.001)));
			t_sample sidechainTrimHistoryNext = fixdenorm(smoothSidechainTrimGain);
			t_sample sidechainTrimGainLinear = dbtoa(smoothSidechainTrimGain);
			t_sample sidechainTrimReference = sidechainTrimGainLinear;
			t_sample leftSidechainTrimmed = (in3 * sidechainTrimReference);
			t_sample rightSidechainTrimmed = (in4 * sidechainTrimReference);
			t_sample expr_3733 = leftSidechainTrimmed;
			t_sample expr_3734 = rightSidechainTrimmed;
			t_sample sidechainSignalMono = ((rightSidechainTrimmed + leftSidechainTrimmed) * ((t_sample)0.707));
			t_sample mix_3815 = (mainSignalMono + (smoothKeyMix * (sidechainSignalMono - mainSignalMono)));
			t_sample keyMixedSignal = mix_3815;
			t_sample ONSIDECHAIN = smoothScEnable;
			t_sample lpfFinalSignal = ((int)0);
			t_sample hpfFinalSignal = ((int)0);
			t_sample omega = safediv((smoothLpfFreq * ((t_sample)6.2831853071796)), samplerate);
			t_sample sn = sin(omega);
			t_sample cs = cos(omega);
			t_sample alpha = ((sn * ((t_sample)0.5)) * ((int)1));
			t_sample b0 = safediv(((int)1), (((int)1) + alpha));
			t_sample a2 = (((((int)1) - cs) * ((t_sample)0.5)) * b0);
			t_sample a1 = ((((int)1) - cs) * b0);
			t_sample b1 = ((((int)-2) * cs) * b0);
			t_sample b2 = ((((int)1) - alpha) * b0);
			t_sample lpfCoeffA2 = a2;
			t_sample lpfCoeffA1 = a1;
			t_sample lpfCoeffA0 = a2;
			t_sample lpfCoeffB1 = b1;
			t_sample lpfCoeffB2 = b2;
			t_sample omega_hpf = safediv((smoothHpfFreq * ((t_sample)6.2831853071796)), samplerate);
			t_sample sn_hpf = sin(omega_hpf);
			t_sample cs_hpf = cos(omega_hpf);
			t_sample alpha_hpf = ((sn_hpf * ((t_sample)0.5)) * ((t_sample)1));
			t_sample b0_hpf = safediv(((int)1), (((int)1) + alpha_hpf));
			t_sample a_hpf = (((((int)1) + cs_hpf) * ((t_sample)0.5)) * b0_hpf);
			t_sample a_neg_hpf = ((-(((int)1) + cs_hpf)) * b0_hpf);
			t_sample b1_hpf = ((((int)-2) * cs_hpf) * b0_hpf);
			t_sample b2_hpf = ((((int)1) - alpha_hpf) * b0_hpf);
			t_sample hpfCoeffA2 = a_hpf;
			t_sample hpfCoeffA1 = a_neg_hpf;
			t_sample hpfCoeffA0 = a_hpf;
			t_sample hpfCoeffB1 = b1_hpf;
			t_sample hpfCoeffB2 = b2_hpf;
			t_sample sidechainMixedSignal = (keyMixedSignal * ((t_sample)0.707));
			t_sample lpfInput = (sidechainMixedSignal * lpfCoeffA2);
			t_sample lpfFromHistory1 = (m_lpfHistory_47 * lpfCoeffA1);
			t_sample lpfFromHistory2 = (m_lpfHistory_49 * lpfCoeffA0);
			t_sample lpfFeedback1 = (m_lpfHistory_48 * lpfCoeffB2);
			t_sample lpfFeedback2 = (m_lpfHistory_46 * lpfCoeffB1);
			t_sample lpfFilteredSignal = (((lpfFromHistory2 + lpfFromHistory1) + lpfInput) - (lpfFeedback2 + lpfFeedback1));
			t_sample lpfHistory1Next = fixdenorm(m_lpfHistory_47);
			t_sample lpfHistory2Next = fixdenorm(m_lpfHistory_46);
			t_sample lpfHistory3Next = fixdenorm(sidechainMixedSignal);
			t_sample lpfHistory4Next = fixdenorm(lpfFilteredSignal);
			t_sample lpfStage2Input = (lpfFilteredSignal * lpfCoeffA2);
			t_sample lpfStage2FromHistory1 = (m_lpfHistory_43 * lpfCoeffA1);
			t_sample lpfStage2FromHistory2 = (m_lpfHistory_45 * lpfCoeffA0);
			t_sample lpfStage2Feedback1 = (m_lpfHistory_44 * lpfCoeffB2);
			t_sample lpfStage2Feedback2 = (m_lpfHistory_42 * lpfCoeffB1);
			t_sample lpfStage2FilteredSignal = (((lpfStage2FromHistory2 + lpfStage2FromHistory1) + lpfStage2Input) - (lpfStage2Feedback2 + lpfStage2Feedback1));
			t_sample lpfHistory5Next = fixdenorm(m_lpfHistory_43);
			t_sample lpfHistory6Next = fixdenorm(m_lpfHistory_42);
			t_sample lpfHistory7Next = fixdenorm(lpfFilteredSignal);
			t_sample lpfHistory8Next = fixdenorm(lpfStage2FilteredSignal);
			t_sample lpfStage3Input = (lpfStage2FilteredSignal * lpfCoeffA2);
			t_sample lpfStage3FromHistory1 = (m_lpfHistory_31 * lpfCoeffA1);
			t_sample lpfStage3FromHistory2 = (m_lpfHistory_33 * lpfCoeffA0);
			t_sample lpfStage3Feedback1 = (m_lpfHistory_32 * lpfCoeffB2);
			t_sample lpfStage3Feedback2 = (m_lpfHistory_30 * lpfCoeffB1);
			t_sample lpfStage3FilteredSignal = (((lpfStage3FromHistory2 + lpfStage3FromHistory1) + lpfStage3Input) - (lpfStage3Feedback2 + lpfStage3Feedback1));
			t_sample lpfHistory9Next = fixdenorm(m_lpfHistory_31);
			t_sample lpfHistory10Next = fixdenorm(m_lpfHistory_30);
			t_sample lpfHistory11Next = fixdenorm(lpfStage2FilteredSignal);
			t_sample lpfHistory12Next = fixdenorm(lpfStage3FilteredSignal);
			t_sample lpfStage4Input = (lpfStage3FilteredSignal * lpfCoeffA2);
			t_sample lpfStage4FromHistory1 = (m_lpfHistory_27 * lpfCoeffA1);
			t_sample lpfStage4FromHistory2 = (m_lpfHistory_29 * lpfCoeffA0);
			t_sample lpfStage4Feedback1 = (m_lpfHistory_28 * lpfCoeffB2);
			t_sample lpfStage4Feedback2 = (m_lpfHistory_26 * lpfCoeffB1);
			t_sample lpfStage4FilteredSignal = (((lpfStage4FromHistory2 + lpfStage4FromHistory1) + lpfStage4Input) - (lpfStage4Feedback2 + lpfStage4Feedback1));
			t_sample lpfHistory13Next = fixdenorm(m_lpfHistory_27);
			t_sample lpfHistory14Next = fixdenorm(m_lpfHistory_26);
			t_sample lpfHistory15Next = fixdenorm(lpfStage3FilteredSignal);
			t_sample lpfHistory16Next = fixdenorm(lpfStage4FilteredSignal);
			if ((lpfOrderDiscrete == ((int)0))) {
				lpfFinalSignal = lpfFilteredSignal;
				
			} else {
				lpfFinalSignal = lpfStage2FilteredSignal;
				
			};
			t_sample mix_3816 = (sidechainMixedSignal + (ONSIDECHAIN * (lpfFinalSignal - sidechainMixedSignal)));
			t_sample lpfMixedSignal = mix_3816;
			t_sample hpfInput = (lpfMixedSignal * hpfCoeffA2);
			t_sample hpfFromHistory1 = (m_hpfHistory_39 * hpfCoeffA1);
			t_sample hpfFromHistory2 = (m_hpfHistory_40 * hpfCoeffA0);
			t_sample hpfFeedback1 = (m_hpfHistory_41 * hpfCoeffB2);
			t_sample hpfFeedback2 = (m_hpfHistory_38 * hpfCoeffB1);
			t_sample hpfFilteredSignal = (((hpfFromHistory2 + hpfFromHistory1) + hpfInput) - (hpfFeedback2 + hpfFeedback1));
			t_sample hpfHistory1Next = fixdenorm(m_hpfHistory_38);
			t_sample hpfHistory2Next = fixdenorm(m_hpfHistory_39);
			t_sample hpfHistory3Next = fixdenorm(lpfMixedSignal);
			t_sample hpfHistory4Next = fixdenorm(hpfFilteredSignal);
			t_sample hpfStage2Input = (hpfFilteredSignal * hpfCoeffA2);
			t_sample hpfStage2FromHistory1 = (m_hpfHistory_35 * hpfCoeffA1);
			t_sample hpfStage2FromHistory2 = (m_hpfHistory_36 * hpfCoeffA0);
			t_sample hpfStage2Feedback1 = (m_hpfHistory_37 * hpfCoeffB2);
			t_sample hpfStage2Feedback2 = (m_hpfHistory_34 * hpfCoeffB1);
			t_sample hpfStage2FilteredSignal = (((hpfStage2FromHistory2 + hpfStage2FromHistory1) + hpfStage2Input) - (hpfStage2Feedback2 + hpfStage2Feedback1));
			t_sample hpfHistory5Next = fixdenorm(m_hpfHistory_34);
			t_sample hpfHistory6Next = fixdenorm(m_hpfHistory_35);
			t_sample hpfHistory7Next = fixdenorm(hpfFilteredSignal);
			t_sample hpfHistory8Next = fixdenorm(hpfStage2FilteredSignal);
			t_sample hpfStage3Input = (hpfStage2FilteredSignal * hpfCoeffA2);
			t_sample hpfStage3FromHistory1 = (m_hpfHistory_23 * hpfCoeffA1);
			t_sample hpfStage3FromHistory2 = (m_hpfHistory_24 * hpfCoeffA0);
			t_sample hpfStage3Feedback1 = (m_hpfHistory_25 * hpfCoeffB2);
			t_sample hpfStage3Feedback2 = (m_hpfHistory_22 * hpfCoeffB1);
			t_sample hpfStage3FilteredSignal = (((hpfStage3FromHistory2 + hpfStage3FromHistory1) + hpfStage3Input) - (hpfStage3Feedback2 + hpfStage3Feedback1));
			t_sample hpfHistory9Next = fixdenorm(m_hpfHistory_22);
			t_sample hpfHistory10Next = fixdenorm(m_hpfHistory_23);
			t_sample hpfHistory11Next = fixdenorm(hpfStage2FilteredSignal);
			t_sample hpfHistory12Next = fixdenorm(hpfStage3FilteredSignal);
			t_sample hpfStage4Input = (hpfStage3FilteredSignal * hpfCoeffA2);
			t_sample hpfStage4FromHistory1 = (m_hpfHistory_19 * hpfCoeffA1);
			t_sample hpfStage4FromHistory2 = (m_hpfHistory_20 * hpfCoeffA0);
			t_sample hpfStage4Feedback1 = (m_hpfHistory_21 * hpfCoeffB2);
			t_sample hpfStage4Feedback2 = (m_hpfHistory_18 * hpfCoeffB1);
			t_sample hpfStage4FilteredSignal = (((hpfStage4FromHistory2 + hpfStage4FromHistory1) + hpfStage4Input) - (hpfStage4Feedback2 + hpfStage4Feedback1));
			t_sample hpfHistory13Next = fixdenorm(m_hpfHistory_18);
			t_sample hpfHistory14Next = fixdenorm(m_hpfHistory_19);
			t_sample hpfHistory15Next = fixdenorm(hpfStage3FilteredSignal);
			t_sample hpfHistory16Next = fixdenorm(hpfStage4FilteredSignal);
			if ((hpfOrderDiscrete == ((int)0))) {
				hpfFinalSignal = hpfFilteredSignal;
				
			} else {
				hpfFinalSignal = hpfStage2FilteredSignal;
				
			};
			t_sample mix_3817 = (lpfMixedSignal + (ONSIDECHAIN * (hpfFinalSignal - lpfMixedSignal)));
			t_sample sidechainProcessedSignal = mix_3817;
			t_sample sidechainDetectionSignal = sidechainProcessedSignal;
			t_sample attack_ms = ((((t_sample)0.05) < smoothAttack) ? smoothAttack : ((t_sample)0.05));
			t_sample release_ms = ((((t_sample)0.1) < smoothRelease) ? smoothRelease : ((t_sample)0.1));
			t_sample attack_coeff = exp(safediv(((int)-1), ((attack_ms * samplerate) * ((t_sample)0.001))));
			t_sample release_coeff = exp(safediv(((int)-1), ((release_ms * samplerate) * ((t_sample)0.001))));
			t_sample attack_coeff_modified = attack_coeff;
			t_sample release_coeff_modified = release_coeff;
			m_peakEnvSharp_9 = fabs(sidechainDetectionSignal);
			t_sample env_post_release = ((int)0);
			t_sample release_env = ((int)0);
			t_sample attack_env_sharp = ((int)0);
			t_sample sharpEnvelopeFinal = ((int)0);
			t_sample hold_samples = (smoothHold * (samplerate * 0.001));
			t_sample envelope_delta_3714 = fabs((m_peakEnvSharp_9 - m_releaseHistSharp_8));
			t_sample sub_3821 = (smoothSensitivity - ((int)0));
			t_sample scale_3818 = ((safepow((sub_3821 * ((t_sample)1)), ((int)1)) * ((t_sample)0.49)) + ((t_sample)0.01));
			t_sample sensitivity_scale_3715 = scale_3818;
			t_sample sensitivity_threshold_3716 = sensitivity_scale_3715;
			int cond_99 = (envelope_delta_3714 > sensitivity_threshold_3716);
			int transient_detected_3717 = (cond_99 ? ((int)1) : ((int)0));
			t_sample holdCounterPlusEqualsNext = m_holdCounterPlusEquals_68;
			if ((transient_detected_3717 > ((int)0))) {
				holdCounterPlusEqualsNext = ((int)0);
				
			} else {
				if ((m_holdCounterPlusEquals_68 < hold_samples)) {
					holdCounterPlusEqualsNext = (m_holdCounterPlusEquals_68 + ((int)1));
					
				};
				
			};
			int cond_100 = (m_holdCounterPlusEquals_68 < hold_samples);
			int still_in_hold = (cond_100 ? ((int)1) : ((int)0));
			int cond_101 = (transient_detected_3717 || still_in_hold);
			int gate_open_3718 = (cond_101 ? ((int)1) : ((int)0));
			if ((gate_open_3718 > ((int)0))) {
				release_env = (m_releaseHistSharp_8 * release_coeff_modified);
				env_post_release = ((m_peakEnvSharp_9 < release_env) ? release_env : m_peakEnvSharp_9);
				attack_env_sharp = (m_attackHistSharp_7 * attack_coeff_modified);
				sharpEnvelopeFinal = (((((int)1) - attack_coeff_modified) * env_post_release) + (attack_coeff_modified * attack_env_sharp));
				
			} else {
				sharpEnvelopeFinal = (m_attackHistSharp_7 * release_coeff_modified);
				
			};
			t_sample sharpEnvelopeFinal_3719 = ((sharpEnvelopeFinal < ((t_sample)1e-06)) ? ((t_sample)1e-06) : sharpEnvelopeFinal);
			m_releaseHistSharp_8 = fixdenorm(env_post_release);
			m_attackHistSharp_7 = fixdenorm(sharpEnvelopeFinal_3719);
			if ((m_z_SMOOTH_90 > ((t_sample)0.01))) {
				t_sample smooth_coeff = (m_z_SMOOTH_90 * ((t_sample)0.99));
				sharpEnvelopeFinal_3719 = ((m_smoothFilterHistory_1 * smooth_coeff) + (sharpEnvelopeFinal_3719 * (((int)1) - smooth_coeff)));
				
			};
			t_sample smoothFilterHistoryNext = sharpEnvelopeFinal_3719;
			int attack_env = gate_open_3718;
			t_sample attack_env_smooth = ((int)0);
			t_sample maxb_102 = (smoothAttack * ((t_sample)0.1));
			t_sample detection_attack_coeff = exp(safediv(((int)-1), (((((((t_sample)0.5) < maxb_102) ? maxb_102 : ((t_sample)0.5)) * samplerate) * ((t_sample)0.001)) * smo_factor)));
			t_sample maxb_103 = (smoothRelease * ((t_sample)0.5));
			t_sample detection_release_coeff = exp(safediv(((int)-1), (((((((int)5) < maxb_103) ? maxb_103 : ((int)5)) * samplerate) * ((t_sample)0.001)) * smo_factor)));
			if ((attack_env > ((t_sample)0.5))) {
				attack_env_smooth = ((m_transientSmoothHistory_74 * detection_attack_coeff) + (attack_env * (((int)1) - detection_attack_coeff)));
				
			} else {
				attack_env_smooth = ((m_transientSmoothHistory_74 * detection_release_coeff) + (attack_env * (((int)1) - detection_release_coeff)));
				
			};
			t_sample transientSmoothHistoryNext = fixdenorm(attack_env_smooth);
			t_sample attack_env_3720 = ((((int)1) < attack_env_smooth) ? ((int)1) : attack_env_smooth);
			t_sample mina_104 = (attack_env_3720 * ((t_sample)1.5));
			t_sample sustain_env = (((int)1) - safepow(((((int)1) < mina_104) ? ((int)1) : mina_104), ((t_sample)0.7)));
			t_sample trimCompensationFactor = safediv(((int)1), trimGainLinear);
			t_sample sidechainTrimCompensated = (sidechainDetectionSignal * trimCompensationFactor);
			t_sample attack_factor = safepow(dbtoa(smoothAttackGain), ((t_sample)0.8));
			t_sample sustain_factor = safepow(dbtoa(smoothSustainGain), ((t_sample)0.8));
			t_sample mul_3823 = (attack_env_3720 * smoothSensitivity);
			t_sample mix_3822 = (((int)1) + (mul_3823 * (attack_factor - ((int)1))));
			t_sample mul_3825 = (sustain_env * smoothSensitivity);
			t_sample mix_3824 = (((int)1) + (mul_3825 * (sustain_factor - ((int)1))));
			t_sample gain_total = (mix_3822 * mix_3824);
			t_sample maxb_105 = ((gain_total < ((int)10)) ? gain_total : ((int)10));
			t_sample gain_total_3721 = ((((t_sample)0.1) < maxb_105) ? maxb_105 : ((t_sample)0.1));
			t_sample expandedLeft = (leftDelayedForCompression * gain_total_3721);
			t_sample delta_control = ((smoothDeltaMode <= ((int)0)) ? ((int)0) : ((smoothDeltaMode >= ((int)1)) ? ((int)1) : smoothDeltaMode));
			t_sample delta_signal = ((int)0);
			if ((delta_control > ((t_sample)0.01))) {
				t_sample delta_mode = smoothRange;
				t_sample mul_3827 = (attack_env_3720 * smoothSensitivity);
				t_sample mix_3826 = (((int)1) + (mul_3827 * (attack_factor - ((int)1))));
				t_sample delta_attack_only = ((leftDelayedForCompression * mix_3826) - leftDelayedForCompression);
				t_sample mul_3829 = (sustain_env * smoothSensitivity);
				t_sample mix_3828 = (((int)1) + (mul_3829 * (sustain_factor - ((int)1))));
				t_sample delta_sustain_only = ((leftDelayedForCompression * mix_3828) - leftDelayedForCompression);
				t_sample delta_both = (expandedLeft - leftDelayedForCompression);
				if ((delta_mode <= ((int)1))) {
					t_sample mix_3830 = (delta_attack_only + (delta_mode * (delta_both - delta_attack_only)));
					delta_signal = mix_3830;
					
				} else {
					delta_mode = (delta_mode - ((int)1));
					t_sample mix_3831 = (delta_both + (delta_mode * (delta_sustain_only - delta_both)));
					delta_signal = mix_3831;
					
				};
				
			};
			t_sample mix_3832 = (expandedLeft + (delta_control * (delta_signal - expandedLeft)));
			t_sample reduction_smooth_mul = (m_reductionHistLeft_6 * ((t_sample)0.999));
			t_sample reduction_smoothed = (((int)0) + reduction_smooth_mul);
			t_sample makeup_linear = dbtoa(smoothMakeupGain);
			t_sample leftWithMakeup = (mix_3832 * makeup_linear);
			m_reductionHistLeft_6 = fixdenorm(reduction_smoothed);
			t_sample mix_3833 = (leftDelayedForMixing + (mix_3814 * (leftWithMakeup - leftDelayedForMixing)));
			t_sample mix_3834 = (mix_3833 + (smoothSoloSidechain * (sidechainDelayedWrite - mix_3833)));
			t_sample leftWithSidechain = mix_3834;
			t_sample saturationAmount = ((smoothSoftclip <= ((int)0)) ? ((int)0) : ((smoothSoftclip >= ((int)1)) ? ((int)1) : smoothSoftclip));
			t_sample leftSaturated = ((int)0);
			if ((leftWithSidechain > ((int)0))) {
				leftSaturated = (tanh((leftWithSidechain * ((t_sample)1.2))) * ((t_sample)0.833));
				
			} else {
				leftSaturated = (tanh((leftWithSidechain * ((t_sample)0.8))) * ((t_sample)1.25));
				
			};
			t_sample leftSaturated_3722 = ((leftSaturated <= ((t_sample)-0.989)) ? ((t_sample)-0.989) : ((leftSaturated >= ((t_sample)0.989)) ? ((t_sample)0.989) : leftSaturated));
			t_sample mix_3835 = (leftWithSidechain + (saturationAmount * (leftSaturated_3722 - leftWithSidechain)));
			t_sample mix_3836 = (leftDelayedForMixing + (smoothBypassAmount * (mix_3835 - leftDelayedForMixing)));
			t_sample expandedRight = (rightDelayedForCompression * gain_total_3721);
			t_sample rightDeltaControl = ((smoothDeltaMode <= ((int)0)) ? ((int)0) : ((smoothDeltaMode >= ((int)1)) ? ((int)1) : smoothDeltaMode));
			t_sample rightDeltaSignal = ((int)0);
			if ((rightDeltaControl > ((t_sample)0.01))) {
				t_sample right_delta_mode = smoothRange;
				t_sample mul_3838 = (attack_env_3720 * smoothSensitivity);
				t_sample mix_3837 = (((int)1) + (mul_3838 * (attack_factor - ((int)1))));
				t_sample right_delta_attack_only = ((rightDelayedForCompression * mix_3837) - rightDelayedForCompression);
				t_sample mul_3840 = (sustain_env * smoothSensitivity);
				t_sample mix_3839 = (((int)1) + (mul_3840 * (sustain_factor - ((int)1))));
				t_sample right_delta_sustain_only = ((rightDelayedForCompression * mix_3839) - rightDelayedForCompression);
				t_sample right_delta_both = (expandedRight - rightDelayedForCompression);
				if ((right_delta_mode <= ((int)1))) {
					t_sample mix_3841 = (right_delta_attack_only + (right_delta_mode * (right_delta_both - right_delta_attack_only)));
					rightDeltaSignal = mix_3841;
					
				} else {
					right_delta_mode = (right_delta_mode - ((int)1));
					t_sample mix_3842 = (right_delta_both + (right_delta_mode * (right_delta_sustain_only - right_delta_both)));
					rightDeltaSignal = mix_3842;
					
				};
				
			};
			t_sample mix_3843 = (expandedRight + (rightDeltaControl * (rightDeltaSignal - expandedRight)));
			t_sample rightReductionSmoothMul = (m_reductionHistRight_5 * ((t_sample)0.999));
			t_sample rightReductionSmoothed = (((int)0) + rightReductionSmoothMul);
			t_sample rightMakeupLinear = dbtoa(smoothMakeupGain);
			t_sample rightWithMakeup = (mix_3843 * rightMakeupLinear);
			m_reductionHistRight_5 = fixdenorm(rightReductionSmoothed);
			t_sample mix_3844 = (rightDelayedForMixing + (mix_3814 * (rightWithMakeup - rightDelayedForMixing)));
			t_sample mix_3845 = (mix_3844 + (smoothSoloSidechain * (sidechainDelayedTap - mix_3844)));
			t_sample rightWithSidechain = mix_3845;
			t_sample rightSaturationAmount = ((smoothSoftclip <= ((int)0)) ? ((int)0) : ((smoothSoftclip >= ((int)1)) ? ((int)1) : smoothSoftclip));
			t_sample rightSaturated = ((int)0);
			if ((rightWithSidechain > ((int)0))) {
				rightSaturated = (tanh((rightWithSidechain * ((t_sample)1.2))) * ((t_sample)0.833));
				
			} else {
				rightSaturated = (tanh((rightWithSidechain * ((t_sample)0.8))) * ((t_sample)1.25));
				
			};
			t_sample rightSaturated_3724 = ((rightSaturated <= ((t_sample)-0.989)) ? ((t_sample)-0.989) : ((rightSaturated >= ((t_sample)0.989)) ? ((t_sample)0.989) : rightSaturated));
			t_sample mix_3846 = (rightWithSidechain + (rightSaturationAmount * (rightSaturated_3724 - rightWithSidechain)));
			t_sample mix_3847 = (rightDelayedForMixing + (smoothBypassAmount * (mix_3846 - rightDelayedForMixing)));
			t_sample attack_gain_db = ((int)0);
			t_sample attack_meter_db = ((int)0);
			t_sample sustain_gain_db = ((int)0);
			t_sample sustain_meter_db = ((int)0);
			if ((attack_env_3720 > ((t_sample)0.001))) {
				attack_gain_db = atodb(mix_3822);
				attack_meter_db = (attack_gain_db * attack_env_3720);
				
			} else {
				attack_meter_db = ((int)0);
				
			};
			t_sample expr_3730 = (attack_meter_db * ((t_sample)0.055555555555556));
			if ((sustain_env > ((t_sample)0.001))) {
				sustain_gain_db = atodb(mix_3824);
				sustain_meter_db = (sustain_gain_db * sustain_env);
				
			} else {
				sustain_meter_db = ((int)0);
				
			};
			t_sample expr_3735 = (sustain_meter_db * ((t_sample)0.055555555555556));
			m_deltaModeHistory_67 = deltaModeHistoryNext;
			m_attackGainHistory_71 = attackGainHistoryNext;
			m_sustainGainHistory_70 = sustainGainHistoryNext;
			m_sensitivityHistory_69 = sensitivityHistoryNext;
			m_makeupGainHistory_66 = makeupGainHistoryNext;
			m_soloSidechainHistory_65 = soloSidechainHistoryNext;
			m_bypassAmountHistory_64 = bypassAmountHistoryNext;
			m_keyMixHistory_63 = keyMixHistoryNext;
			m_dryWetMixHistory_62 = dryWetMixHistoryNext;
			m_trimHistory_61 = trimHistoryNext;
			m_sidechainTrimHistory_60 = sidechainTrimHistoryNext;
			m_rangeHistory_59 = rangeHistoryNext;
			m_smoothAmountHistory_58 = smoothAmountHistoryNext;
			m_lookaheadHistory_57 = lookaheadHistoryNext;
			m_hpfFreqHistory_56 = hpfFreqHistoryNext;
			m_lpfFreqHistory_55 = lpfFreqHistoryNext;
			m_scEnableHistory_54 = scEnableHistoryNext;
			m_softclipHistory_53 = softclipHistoryNext;
			m_atkHistory_52 = atkHistoryNext;
			m_relHistory_51 = relHistoryNext;
			m_holdHistory_50 = holdHistoryNext;
			m_smoothFilterHistory_1 = smoothFilterHistoryNext;
			m_transientSmoothHistory_74 = transientSmoothHistoryNext;
			m_lpfHistory_49 = lpfHistory1Next;
			m_lpfHistory_48 = lpfHistory2Next;
			m_lpfHistory_47 = lpfHistory3Next;
			m_lpfHistory_46 = lpfHistory4Next;
			m_lpfHistory_45 = lpfHistory5Next;
			m_lpfHistory_44 = lpfHistory6Next;
			m_lpfHistory_43 = lpfHistory7Next;
			m_lpfHistory_42 = lpfHistory8Next;
			m_lpfHistory_33 = lpfHistory9Next;
			m_lpfHistory_32 = lpfHistory10Next;
			m_lpfHistory_31 = lpfHistory11Next;
			m_lpfHistory_30 = lpfHistory12Next;
			m_lpfHistory_29 = lpfHistory13Next;
			m_lpfHistory_28 = lpfHistory14Next;
			m_lpfHistory_27 = lpfHistory15Next;
			m_lpfHistory_26 = lpfHistory16Next;
			m_hpfHistory_41 = hpfHistory1Next;
			m_hpfHistory_40 = hpfHistory2Next;
			m_hpfHistory_39 = hpfHistory3Next;
			m_hpfHistory_38 = hpfHistory4Next;
			m_hpfHistory_37 = hpfHistory5Next;
			m_hpfHistory_36 = hpfHistory6Next;
			m_hpfHistory_35 = hpfHistory7Next;
			m_hpfHistory_34 = hpfHistory8Next;
			m_hpfHistory_25 = hpfHistory9Next;
			m_hpfHistory_24 = hpfHistory10Next;
			m_hpfHistory_23 = hpfHistory11Next;
			m_hpfHistory_22 = hpfHistory12Next;
			m_hpfHistory_21 = hpfHistory13Next;
			m_hpfHistory_20 = hpfHistory14Next;
			m_hpfHistory_19 = hpfHistory15Next;
			m_hpfHistory_18 = hpfHistory16Next;
			m_hpfOrderHistory_17 = hpfOrderHistoryNext;
			m_lpfOrderHistory_16 = lpfOrderHistoryNext;
			m_holdCounterPlusEquals_68 = holdCounterPlusEqualsNext;
			m_sidechainWriteDelay_10.write(sidechainTrimCompensated);
			m_sidechainTapDelay_11.write(sidechainTrimCompensated);
			t_sample out8 = expr_3735;
			t_sample out7 = expr_3734;
			t_sample out5 = rightDelayedForCompression;
			t_sample out4 = leftDelayedForCompression;
			t_sample out6 = expr_3733;
			t_sample out1 = mix_3836;
			t_sample out2 = mix_3847;
			t_sample out3 = expr_3730;
			m_sidechainWriteDelay_10.step();
			m_sidechainTapDelay_11.step();
			m_rightInputDelay_12.step();
			m_leftInputDelay_13.step();
			m_rightOutputDelay_14.step();
			m_leftOutputDelay_15.step();
			// assign results to output buffer;
			(*(__out1++)) = out1;
			(*(__out2++)) = out2;
			(*(__out3++)) = out3;
			(*(__out4++)) = out4;
			(*(__out5++)) = out5;
			(*(__out6++)) = out6;
			(*(__out7++)) = out7;
			(*(__out8++)) = out8;
			
		};
		return __exception;
		
	};
	inline void set_p_BYPASS(t_param _value) {
		m_p_BYPASS_76 = (_value < 0 ? 0 : (_value > 1 ? 1 : _value));
	};
	inline void set_u_SOFTCLIP(t_param _value) {
		m_u_SOFTCLIP_77 = (_value < 0 ? 0 : (_value > 1 ? 1 : _value));
	};
	inline void set_n_LOOKAHEAD(t_param _value) {
		m_n_LOOKAHEAD_78 = (_value < 0 ? 0 : (_value > 10 ? 10 : _value));
	};
	inline void set_h_DELTAMODE(t_param _value) {
		m_h_DELTAMODE_79 = (_value < 0 ? 0 : (_value > 2 ? 2 : _value));
	};
	inline void set_v_DELTA(t_param _value) {
		m_v_DELTA_80 = (_value < 0 ? 0 : (_value > 1 ? 1 : _value));
	};
	inline void set_o_DRYWET(t_param _value) {
		m_o_DRYWET_81 = (_value < 0 ? 0 : (_value > 1 ? 1 : _value));
	};
	inline void set_m_SOLOSC(t_param _value) {
		m_m_SOLOSC_82 = (_value < 0 ? 0 : (_value > 1 ? 1 : _value));
	};
	inline void set_r_KEY(t_param _value) {
		m_r_KEY_83 = (_value < 0 ? 0 : (_value > 1 ? 1 : _value));
	};
	inline void set_y_SCTRIM(t_param _value) {
		m_y_SCTRIM_84 = (_value < -12 ? -12 : (_value > 12 ? 12 : _value));
	};
	inline void set_l_SC(t_param _value) {
		m_l_SC_85 = (_value < 0 ? 0 : (_value > 1 ? 1 : _value));
	};
	inline void set_k_LPFORDER(t_param _value) {
		m_k_LPFORDER_86 = (_value < 0 ? 0 : (_value > 1 ? 1 : _value));
	};
	inline void set_k_LPF(t_param _value) {
		m_k_LPF_87 = (_value < 20 ? 20 : (_value > 20000 ? 20000 : _value));
	};
	inline void set_j_HPFORDER(t_param _value) {
		m_j_HPFORDER_88 = (_value < 0 ? 0 : (_value > 1 ? 1 : _value));
	};
	inline void set_j_HPF(t_param _value) {
		m_j_HPF_89 = (_value < 20 ? 20 : (_value > 20000 ? 20000 : _value));
	};
	inline void set_z_SMOOTH(t_param _value) {
		m_z_SMOOTH_90 = (_value < 0 ? 0 : (_value > 1 ? 1 : _value));
	};
	inline void set_i_MAKEUP(t_param _value) {
		m_i_MAKEUP_91 = (_value < -12 ? -12 : (_value > 12 ? 12 : _value));
	};
	inline void set_e_REL(t_param _value) {
		m_e_REL_92 = (_value < 0.1 ? 0.1 : (_value > 350 ? 350 : _value));
	};
	inline void set_f_HOLD(t_param _value) {
		m_f_HOLD_93 = (_value < 0 ? 0 : (_value > 250 ? 250 : _value));
	};
	inline void set_d_ATK(t_param _value) {
		m_d_ATK_94 = (_value < 0 ? 0 : (_value > 150 ? 150 : _value));
	};
	inline void set_q_SENSITIVITY(t_param _value) {
		m_q_SENSITIVITY_95 = (_value < 0 ? 0 : (_value > 1 ? 1 : _value));
	};
	inline void set_c_SUSTAIN_GAIN(t_param _value) {
		m_c_SUSTAIN_GAIN_96 = (_value < -18 ? -18 : (_value > 18 ? 18 : _value));
	};
	inline void set_b_ATTACK_GAIN(t_param _value) {
		m_b_ATTACK_GAIN_97 = (_value < -18 ? -18 : (_value > 18 ? 18 : _value));
	};
	inline void set_a_TRIM(t_param _value) {
		m_a_TRIM_98 = (_value < -12 ? -12 : (_value > 12 ? 12 : _value));
	};
	
} State;


///
///	Configuration for the genlib API
///

/// Number of signal inputs and outputs

int gen_kernel_numins = 4;
int gen_kernel_numouts = 8;

int num_inputs() { return gen_kernel_numins; }
int num_outputs() { return gen_kernel_numouts; }
int num_params() { return 23; }

/// Assistive lables for the signal inputs and outputs

const char *gen_kernel_innames[] = { "in1", "in2", "in3", "in4" };
const char *gen_kernel_outnames[] = { "out1", "out2", "out3", "out4", "out5", "out6", "out7", "out8" };

/// Invoke the signal process of a State object

int perform(CommonState *cself, t_sample **ins, long numins, t_sample **outs, long numouts, long n) {
	State* self = (State *)cself;
	return self->perform(ins, outs, n);
}

/// Reset all parameters and stateful operators of a State object

void reset(CommonState *cself) {
	State* self = (State *)cself;
	self->reset(cself->sr, cself->vs);
}

/// Set a parameter of a State object

void setparameter(CommonState *cself, long index, t_param value, void *ref) {
	State *self = (State *)cself;
	switch (index) {
		case 0: self->set_a_TRIM(value); break;
		case 1: self->set_b_ATTACK_GAIN(value); break;
		case 2: self->set_c_SUSTAIN_GAIN(value); break;
		case 3: self->set_d_ATK(value); break;
		case 4: self->set_e_REL(value); break;
		case 5: self->set_f_HOLD(value); break;
		case 6: self->set_h_DELTAMODE(value); break;
		case 7: self->set_i_MAKEUP(value); break;
		case 8: self->set_j_HPF(value); break;
		case 9: self->set_j_HPFORDER(value); break;
		case 10: self->set_k_LPF(value); break;
		case 11: self->set_k_LPFORDER(value); break;
		case 12: self->set_l_SC(value); break;
		case 13: self->set_m_SOLOSC(value); break;
		case 14: self->set_n_LOOKAHEAD(value); break;
		case 15: self->set_o_DRYWET(value); break;
		case 16: self->set_p_BYPASS(value); break;
		case 17: self->set_q_SENSITIVITY(value); break;
		case 18: self->set_r_KEY(value); break;
		case 19: self->set_u_SOFTCLIP(value); break;
		case 20: self->set_v_DELTA(value); break;
		case 21: self->set_y_SCTRIM(value); break;
		case 22: self->set_z_SMOOTH(value); break;
		
		default: break;
	}
}

/// Get the value of a parameter of a State object

void getparameter(CommonState *cself, long index, t_param *value) {
	State *self = (State *)cself;
	switch (index) {
		case 0: *value = self->m_a_TRIM_98; break;
		case 1: *value = self->m_b_ATTACK_GAIN_97; break;
		case 2: *value = self->m_c_SUSTAIN_GAIN_96; break;
		case 3: *value = self->m_d_ATK_94; break;
		case 4: *value = self->m_e_REL_92; break;
		case 5: *value = self->m_f_HOLD_93; break;
		case 6: *value = self->m_h_DELTAMODE_79; break;
		case 7: *value = self->m_i_MAKEUP_91; break;
		case 8: *value = self->m_j_HPF_89; break;
		case 9: *value = self->m_j_HPFORDER_88; break;
		case 10: *value = self->m_k_LPF_87; break;
		case 11: *value = self->m_k_LPFORDER_86; break;
		case 12: *value = self->m_l_SC_85; break;
		case 13: *value = self->m_m_SOLOSC_82; break;
		case 14: *value = self->m_n_LOOKAHEAD_78; break;
		case 15: *value = self->m_o_DRYWET_81; break;
		case 16: *value = self->m_p_BYPASS_76; break;
		case 17: *value = self->m_q_SENSITIVITY_95; break;
		case 18: *value = self->m_r_KEY_83; break;
		case 19: *value = self->m_u_SOFTCLIP_77; break;
		case 20: *value = self->m_v_DELTA_80; break;
		case 21: *value = self->m_y_SCTRIM_84; break;
		case 22: *value = self->m_z_SMOOTH_90; break;
		
		default: break;
	}
}

/// Get the name of a parameter of a State object

const char *getparametername(CommonState *cself, long index) {
	if (index >= 0 && index < cself->numparams) {
		return cself->params[index].name;
	}
	return 0;
}

/// Get the minimum value of a parameter of a State object

t_param getparametermin(CommonState *cself, long index) {
	if (index >= 0 && index < cself->numparams) {
		return cself->params[index].outputmin;
	}
	return 0;
}

/// Get the maximum value of a parameter of a State object

t_param getparametermax(CommonState *cself, long index) {
	if (index >= 0 && index < cself->numparams) {
		return cself->params[index].outputmax;
	}
	return 0;
}

/// Get parameter of a State object has a minimum and maximum value

char getparameterhasminmax(CommonState *cself, long index) {
	if (index >= 0 && index < cself->numparams) {
		return cself->params[index].hasminmax;
	}
	return 0;
}

/// Get the units of a parameter of a State object

const char *getparameterunits(CommonState *cself, long index) {
	if (index >= 0 && index < cself->numparams) {
		return cself->params[index].units;
	}
	return 0;
}

/// Get the size of the state of all parameters of a State object

size_t getstatesize(CommonState *cself) {
	return genlib_getstatesize(cself, &getparameter);
}

/// Get the state of all parameters of a State object

short getstate(CommonState *cself, char *state) {
	return genlib_getstate(cself, state, &getparameter);
}

/// set the state of all parameters of a State object

short setstate(CommonState *cself, const char *state) {
	return genlib_setstate(cself, state, &setparameter);
}

/// Allocate and configure a new State object and it's internal CommonState:

void *create(t_param sr, long vs) {
	State *self = new State;
	self->reset(sr, vs);
	ParamInfo *pi;
	self->__commonstate.inputnames = gen_kernel_innames;
	self->__commonstate.outputnames = gen_kernel_outnames;
	self->__commonstate.numins = gen_kernel_numins;
	self->__commonstate.numouts = gen_kernel_numouts;
	self->__commonstate.sr = sr;
	self->__commonstate.vs = vs;
	self->__commonstate.params = (ParamInfo *)genlib_sysmem_newptr(23 * sizeof(ParamInfo));
	self->__commonstate.numparams = 23;
	// initialize parameter 0 ("m_a_TRIM_98")
	pi = self->__commonstate.params + 0;
	pi->name = "a_TRIM";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_a_TRIM_98;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = -12;
	pi->outputmax = 12;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 1 ("m_b_ATTACK_GAIN_97")
	pi = self->__commonstate.params + 1;
	pi->name = "b_ATTACK_GAIN";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_b_ATTACK_GAIN_97;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = -18;
	pi->outputmax = 18;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 2 ("m_c_SUSTAIN_GAIN_96")
	pi = self->__commonstate.params + 2;
	pi->name = "c_SUSTAIN_GAIN";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_c_SUSTAIN_GAIN_96;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = -18;
	pi->outputmax = 18;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 3 ("m_d_ATK_94")
	pi = self->__commonstate.params + 3;
	pi->name = "d_ATK";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_d_ATK_94;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 150;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 4 ("m_e_REL_92")
	pi = self->__commonstate.params + 4;
	pi->name = "e_REL";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_e_REL_92;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0.1;
	pi->outputmax = 350;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 5 ("m_f_HOLD_93")
	pi = self->__commonstate.params + 5;
	pi->name = "f_HOLD";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_f_HOLD_93;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 250;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 6 ("m_h_DELTAMODE_79")
	pi = self->__commonstate.params + 6;
	pi->name = "h_DELTAMODE";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_h_DELTAMODE_79;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 2;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 7 ("m_i_MAKEUP_91")
	pi = self->__commonstate.params + 7;
	pi->name = "i_MAKEUP";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_i_MAKEUP_91;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = -12;
	pi->outputmax = 12;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 8 ("m_j_HPF_89")
	pi = self->__commonstate.params + 8;
	pi->name = "j_HPF";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_j_HPF_89;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 20;
	pi->outputmax = 20000;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 9 ("m_j_HPFORDER_88")
	pi = self->__commonstate.params + 9;
	pi->name = "j_HPFORDER";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_j_HPFORDER_88;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 1;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 10 ("m_k_LPF_87")
	pi = self->__commonstate.params + 10;
	pi->name = "k_LPF";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_k_LPF_87;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 20;
	pi->outputmax = 20000;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 11 ("m_k_LPFORDER_86")
	pi = self->__commonstate.params + 11;
	pi->name = "k_LPFORDER";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_k_LPFORDER_86;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 1;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 12 ("m_l_SC_85")
	pi = self->__commonstate.params + 12;
	pi->name = "l_SC";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_l_SC_85;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 1;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 13 ("m_m_SOLOSC_82")
	pi = self->__commonstate.params + 13;
	pi->name = "m_SOLOSC";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_m_SOLOSC_82;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 1;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 14 ("m_n_LOOKAHEAD_78")
	pi = self->__commonstate.params + 14;
	pi->name = "n_LOOKAHEAD";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_n_LOOKAHEAD_78;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 10;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 15 ("m_o_DRYWET_81")
	pi = self->__commonstate.params + 15;
	pi->name = "o_DRYWET";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_o_DRYWET_81;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 1;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 16 ("m_p_BYPASS_76")
	pi = self->__commonstate.params + 16;
	pi->name = "p_BYPASS";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_p_BYPASS_76;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 1;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 17 ("m_q_SENSITIVITY_95")
	pi = self->__commonstate.params + 17;
	pi->name = "q_SENSITIVITY";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_q_SENSITIVITY_95;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 1;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 18 ("m_r_KEY_83")
	pi = self->__commonstate.params + 18;
	pi->name = "r_KEY";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_r_KEY_83;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 1;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 19 ("m_u_SOFTCLIP_77")
	pi = self->__commonstate.params + 19;
	pi->name = "u_SOFTCLIP";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_u_SOFTCLIP_77;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 1;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 20 ("m_v_DELTA_80")
	pi = self->__commonstate.params + 20;
	pi->name = "v_DELTA";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_v_DELTA_80;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 1;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 21 ("m_y_SCTRIM_84")
	pi = self->__commonstate.params + 21;
	pi->name = "y_SCTRIM";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_y_SCTRIM_84;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = -12;
	pi->outputmax = 12;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 22 ("m_z_SMOOTH_90")
	pi = self->__commonstate.params + 22;
	pi->name = "z_SMOOTH";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_z_SMOOTH_90;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 1;
	pi->exp = 0;
	pi->units = "";		// no units defined
	
	return self;
}

/// Release all resources and memory used by a State object:

void destroy(CommonState *cself) {
	State *self = (State *)cself;
	genlib_sysmem_freeptr(cself->params);
		
	delete self;
}


} // JCBTransient::
