#include "JCBExpander.h"

namespace JCBExpander {

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
//static const int GENLIB_LOOPCOUNT_BAIL = 100000;


// The State struct contains all the state and procedures for the gendsp kernel
typedef struct State {
	CommonState __commonstate;
	Delay m_sidechainTapDelay_12;
	Delay m_rightInputDelay_13;
	Delay m_sidechainWriteDelay_11;
	Delay m_leftInputDelay_14;
	Delay m_leftOutputDelay_16;
	Delay m_rightOutputDelay_15;
	Delay m_rmsDelay_10;
	int __exception;
	int vectorsize;
	t_sample m_dryWetMixHistory_65;
	t_sample m_trimHistory_64;
	t_sample m_sidechainTrimHistory_63;
	t_sample m_ratioHistory_62;
	t_sample m_keyMixHistory_66;
	t_sample m_soloSidechainHistory_68;
	t_sample m_bypassAmountHistory_67;
	t_sample m_thresholdHistory_70;
	t_sample m_makeupGainHistory_69;
	t_sample m_lookaheadHistory_57;
	t_sample m_smoothAmountHistory_58;
	t_sample m_softclipHistory_53;
	t_sample m_rangeHistory_61;
	t_sample m_kneeExpHistory_60;
	t_sample m_reactHistory_59;
	t_sample m_atkHistory_52;
	t_sample m_hpfFreqHistory_56;
	t_sample m_deltaModeHistory_71;
	t_sample m_lpfFreqHistory_55;
	t_sample m_scEnableHistory_54;
	t_sample m_p_BYPASS_72;
	t_sample m_o_DRYWET_76;
	t_sample m_n_LOOKAHEAD_74;
	t_sample m_h_RANGE_88;
	t_sample m_q_KNEE_87;
	t_sample m_g_REACT_86;
	t_sample m_z_SMOOTH_85;
	t_sample m_i_MAKEUP_89;
	t_sample m_e_REL_91;
	t_sample m_f_HOLD_90;
	t_sample m_c_RATIO_93;
	t_sample m_d_ATK_92;
	t_sample m_u_SOFTCLIP_73;
	t_sample m_j_HPF_84;
	t_sample m_k_LPF_82;
	t_sample m_m_SOLOSC_77;
	t_sample m_relHistory_51;
	t_sample m_v_DELTA_75;
	t_sample m_j_HPFORDER_83;
	t_sample m_r_KEY_78;
	t_sample m_l_SC_80;
	t_sample m_y_SCTRIM_79;
	t_sample m_k_LPFORDER_81;
	t_sample m_lpfHistory_50;
	t_sample m_lpfHistory_46;
	t_sample m_lpfHistory_48;
	t_sample m_hpfHistory_20;
	t_sample m_hpfHistory_19;
	t_sample m_hpfOrderHistory_18;
	t_sample m_lpfOrderHistory_17;
	t_sample m_hpfHistory_21;
	t_sample m_hpfHistory_23;
	t_sample m_hpfHistory_22;
	t_sample m_hpfHistory_25;
	t_sample m_hpfHistory_24;
	t_sample m_hpfHistory_26;
	t_sample m_rmsSum_9;
	t_sample m_releaseHistSharp_7;
	t_sample m_held_peak_sharp_2;
	t_sample m_hold_counter_sharp_1;
	t_sample samplerate;
	t_sample m_peakEnvSharp_8;
	t_sample m_reductionHistRight_3;
	t_sample m_extraSmoothHistSharp_5;
	t_sample m_reductionHistLeft_4;
	t_sample m_attackHistSharp_6;
	t_sample m_lpfHistory_49;
	t_sample m_lpfHistory_27;
	t_sample m_lpfHistory_29;
	t_sample m_lpfHistory_43;
	t_sample m_hpfHistory_42;
	t_sample m_hpfHistory_41;
	t_sample m_hpfHistory_40;
	t_sample m_lpfHistory_44;
	t_sample m_b_THD_94;
	t_sample m_lpfHistory_45;
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
	t_sample m_lpfHistory_34;
	t_sample m_hpfHistory_36;
	t_sample m_a_TRIM_95;
	// re-initialize all member variables;
	inline void reset(t_param __sr, int __vs) {
		__exception = 0;
		vectorsize = __vs;
		samplerate = __sr;
		m_hold_counter_sharp_1 = ((int)0);
		m_held_peak_sharp_2 = ((int)0);
		m_reductionHistRight_3 = ((int)0);
		m_reductionHistLeft_4 = ((int)0);
		m_extraSmoothHistSharp_5 = ((int)0);
		m_attackHistSharp_6 = ((int)0);
		m_releaseHistSharp_7 = ((int)0);
		m_peakEnvSharp_8 = ((int)0);
		m_rmsSum_9 = ((int)0);
		m_rmsDelay_10.reset("m_rmsDelay_10", ((int)500));
		m_sidechainWriteDelay_11.reset("m_sidechainWriteDelay_11", (((t_sample)0.025) * samplerate));
		m_sidechainTapDelay_12.reset("m_sidechainTapDelay_12", (((t_sample)0.025) * samplerate));
		m_rightInputDelay_13.reset("m_rightInputDelay_13", (((t_sample)0.025) * samplerate));
		m_leftInputDelay_14.reset("m_leftInputDelay_14", (((t_sample)0.025) * samplerate));
		m_rightOutputDelay_15.reset("m_rightOutputDelay_15", (((t_sample)0.025) * samplerate));
		m_leftOutputDelay_16.reset("m_leftOutputDelay_16", (((t_sample)0.025) * samplerate));
		m_lpfOrderHistory_17 = ((int)0);
		m_hpfOrderHistory_18 = ((int)0);
		m_hpfHistory_19 = ((int)0);
		m_hpfHistory_20 = ((int)0);
		m_hpfHistory_21 = ((int)0);
		m_hpfHistory_22 = ((int)0);
		m_hpfHistory_23 = ((int)0);
		m_hpfHistory_24 = ((int)0);
		m_hpfHistory_25 = ((int)0);
		m_hpfHistory_26 = ((int)0);
		m_lpfHistory_27 = ((int)0);
		m_lpfHistory_28 = ((int)0);
		m_lpfHistory_29 = ((int)0);
		m_lpfHistory_30 = ((int)0);
		m_lpfHistory_31 = ((int)0);
		m_lpfHistory_32 = ((int)0);
		m_lpfHistory_33 = ((int)0);
		m_lpfHistory_34 = ((int)0);
		m_hpfHistory_35 = ((int)0);
		m_hpfHistory_36 = ((int)0);
		m_hpfHistory_37 = ((int)0);
		m_hpfHistory_38 = ((int)0);
		m_hpfHistory_39 = ((int)0);
		m_hpfHistory_40 = ((int)0);
		m_hpfHistory_41 = ((int)0);
		m_hpfHistory_42 = ((int)0);
		m_lpfHistory_43 = ((int)0);
		m_lpfHistory_44 = ((int)0);
		m_lpfHistory_45 = ((int)0);
		m_lpfHistory_46 = ((int)0);
		m_lpfHistory_47 = ((int)0);
		m_lpfHistory_48 = ((int)0);
		m_lpfHistory_49 = ((int)0);
		m_lpfHistory_50 = ((int)0);
		m_relHistory_51 = ((int)0);
		m_atkHistory_52 = ((int)0);
		m_softclipHistory_53 = ((int)0);
		m_scEnableHistory_54 = ((int)0);
		m_lpfFreqHistory_55 = ((int)0);
		m_hpfFreqHistory_56 = ((int)0);
		m_lookaheadHistory_57 = ((int)0);
		m_smoothAmountHistory_58 = ((int)0);
		m_reactHistory_59 = ((int)0);
		m_kneeExpHistory_60 = ((int)0);
		m_rangeHistory_61 = ((int)0);
		m_ratioHistory_62 = ((int)0);
		m_sidechainTrimHistory_63 = ((int)0);
		m_trimHistory_64 = ((int)0);
		m_dryWetMixHistory_65 = ((int)0);
		m_keyMixHistory_66 = ((int)0);
		m_bypassAmountHistory_67 = ((int)0);
		m_soloSidechainHistory_68 = ((int)0);
		m_makeupGainHistory_69 = ((int)0);
		m_thresholdHistory_70 = ((int)0);
		m_deltaModeHistory_71 = ((int)0);
		m_p_BYPASS_72 = 0;
		m_u_SOFTCLIP_73 = 0;
		m_n_LOOKAHEAD_74 = 0;
		m_v_DELTA_75 = 0;
		m_o_DRYWET_76 = 1;
		m_m_SOLOSC_77 = 0;
		m_r_KEY_78 = 0;
		m_y_SCTRIM_79 = 0;
		m_l_SC_80 = 0;
		m_k_LPFORDER_81 = 0;
		m_k_LPF_82 = 20000;
		m_j_HPFORDER_83 = 0;
		m_j_HPF_84 = 20;
		m_z_SMOOTH_85 = 0;
		m_g_REACT_86 = 0;
		m_q_KNEE_87 = 1;
		m_h_RANGE_88 = -20;
		m_i_MAKEUP_89 = 0;
		m_f_HOLD_90 = 0;
		m_e_REL_91 = 120;
		m_d_ATK_92 = 1;
		m_c_RATIO_93 = 4;
		m_b_THD_94 = -18;
		m_a_TRIM_95 = 0;
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
		if (__exception) {
			return __exception;
			
		} else if (( (__in1 == 0) || (__in2 == 0) || (__in3 == 0) || (__in4 == 0) || (__out1 == 0) || (__out2 == 0) || (__out3 == 0) || (__out4 == 0) || (__out5 == 0) || (__out6 == 0) || (__out7 == 0) )) {
			__exception = GENLIB_ERR_NULL_BUFFER;
			return __exception;
			
		};
		t_sample bypassInverted = (((int)1) - m_p_BYPASS_72);
		t_sample maxb_96 = floor((((int)25) * (samplerate * ((t_sample)2.0833333333333e-05))));
		t_sample rms_window = ((((int)1) < maxb_96) ? maxb_96 : ((int)1));
		t_sample rms_window_inv = safediv(((int)1), rms_window);
		t_sample hold_samples = (m_f_HOLD_90 * (samplerate * 0.001));
		// the main sample loop;
		while ((__n--)) {
			const t_sample in1 = (*(__in1++));
			const t_sample in2 = (*(__in2++));
			const t_sample in3 = (*(__in3++));
			const t_sample in4 = (*(__in4++));
			t_sample smoothDeltaMode = ((m_deltaModeHistory_71 * ((t_sample)0.999)) + (m_v_DELTA_75 * ((t_sample)0.001)));
			t_sample deltaModeHistoryNext = fixdenorm(smoothDeltaMode);
			t_sample smoothThresholdDb = ((m_thresholdHistory_70 * ((t_sample)0.999)) + (m_b_THD_94 * ((t_sample)0.001)));
			t_sample thresholdHistoryNext = fixdenorm(smoothThresholdDb);
			t_sample smoothHpfOrder = ((m_hpfOrderHistory_18 * ((t_sample)0.999)) + (m_j_HPFORDER_83 * ((t_sample)0.001)));
			t_sample hpfOrderHistoryNext = fixdenorm(smoothHpfOrder);
			t_sample smoothLpfOrder = ((m_lpfOrderHistory_17 * ((t_sample)0.999)) + (m_k_LPFORDER_81 * ((t_sample)0.001)));
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
			t_sample smoothRatio = ((m_ratioHistory_62 * ((t_sample)0.999)) + (m_c_RATIO_93 * ((t_sample)0.001)));
			t_sample ratioHistoryNext = fixdenorm(smoothRatio);
			t_sample smoothRange = ((m_rangeHistory_61 * ((t_sample)0.999)) + (m_h_RANGE_88 * ((t_sample)0.001)));
			t_sample rangeHistoryNext = fixdenorm(smoothRange);
			t_sample smoothKneeExp = ((m_kneeExpHistory_60 * ((t_sample)0.999)) + (m_q_KNEE_87 * ((t_sample)0.001)));
			t_sample kneeExpHistoryNext = fixdenorm(smoothKneeExp);
			t_sample smoothReact = ((m_reactHistory_59 * ((t_sample)0.999)) + (m_g_REACT_86 * ((t_sample)0.001)));
			t_sample reactHistoryNext = fixdenorm(smoothReact);
			t_sample smoothSmoothAmount = ((m_smoothAmountHistory_58 * ((t_sample)0.999)) + (m_z_SMOOTH_85 * ((t_sample)0.001)));
			t_sample smoothAmountHistoryNext = fixdenorm(smoothSmoothAmount);
			t_sample smoothAttack = ((m_atkHistory_52 * ((t_sample)0.999)) + (m_d_ATK_92 * ((t_sample)0.001)));
			t_sample atkHistoryNext = fixdenorm(smoothAttack);
			t_sample smoothRelease = ((m_relHistory_51 * ((t_sample)0.999)) + (m_e_REL_91 * ((t_sample)0.001)));
			t_sample relHistoryNext = fixdenorm(smoothRelease);
			t_sample smoothHpfFreq = ((m_hpfFreqHistory_56 * ((t_sample)0.999)) + (m_j_HPF_84 * ((t_sample)0.001)));
			t_sample hpfFreqHistoryNext = fixdenorm(smoothHpfFreq);
			t_sample smoothLpfFreq = ((m_lpfFreqHistory_55 * ((t_sample)0.999)) + (m_k_LPF_82 * ((t_sample)0.001)));
			t_sample lpfFreqHistoryNext = fixdenorm(smoothLpfFreq);
			t_sample smoothScEnable = ((m_scEnableHistory_54 * ((t_sample)0.999)) + (m_l_SC_80 * ((t_sample)0.001)));
			t_sample scEnableHistoryNext = fixdenorm(smoothScEnable);
			t_sample smoothSoftclip = ((m_softclipHistory_53 * ((t_sample)0.999)) + (m_u_SOFTCLIP_73 * ((t_sample)0.001)));
			t_sample softclipHistoryNext = fixdenorm(smoothSoftclip);
			t_sample smoothLookahead = ((m_lookaheadHistory_57 * ((t_sample)0.9999)) + (m_n_LOOKAHEAD_74 * ((t_sample)9.9999999999989e-05)));
			t_sample lookaheadHistoryNext = fixdenorm(smoothLookahead);
			t_sample lookaheadSamples = (smoothLookahead * (samplerate * 0.001));
			t_sample leftDelayedForCompression = m_leftOutputDelay_16.read_step(lookaheadSamples);
			t_sample rightDelayedForCompression = m_rightOutputDelay_15.read_step(lookaheadSamples);
			t_sample leftDelayedForMixing = m_leftInputDelay_14.read_step(lookaheadSamples);
			t_sample rightDelayedForMixing = m_rightInputDelay_13.read_step(lookaheadSamples);
			t_sample smoothMakeupGain = ((m_makeupGainHistory_69 * ((t_sample)0.999)) + (m_i_MAKEUP_89 * ((t_sample)0.001)));
			t_sample makeupGainHistoryNext = fixdenorm(smoothMakeupGain);
			t_sample sidechainDelayedTap = m_sidechainTapDelay_12.read_step(lookaheadSamples);
			t_sample smoothSoloSidechain = ((m_soloSidechainHistory_68 * ((t_sample)0.999)) + (m_m_SOLOSC_77 * ((t_sample)0.001)));
			t_sample soloSidechainHistoryNext = fixdenorm(smoothSoloSidechain);
			t_sample sidechainDelayedWrite = m_sidechainWriteDelay_11.read_step(lookaheadSamples);
			t_sample smoothBypassAmount = ((m_bypassAmountHistory_67 * ((t_sample)0.999)) + (bypassInverted * ((t_sample)0.001)));
			t_sample bypassAmountHistoryNext = fixdenorm(smoothBypassAmount);
			t_sample smoothKeyMix = ((m_keyMixHistory_66 * ((t_sample)0.999)) + (m_r_KEY_78 * ((t_sample)0.001)));
			t_sample keyMixHistoryNext = fixdenorm(smoothKeyMix);
			t_sample smoothDryWetMix = ((m_dryWetMixHistory_65 * ((t_sample)0.999)) + (m_o_DRYWET_76 * ((t_sample)0.001)));
			t_sample dryWetMixHistoryNext = fixdenorm(smoothDryWetMix);
			t_sample mix_661 = (smoothDryWetMix + (smoothDeltaMode * (((int)1) - smoothDryWetMix)));
			t_sample smoothTrimGain = ((m_trimHistory_64 * ((t_sample)0.999)) + (m_a_TRIM_95 * ((t_sample)0.001)));
			t_sample trimHistoryNext = fixdenorm(smoothTrimGain);
			t_sample trimGainLinear = dbtoa(smoothTrimGain);
			t_sample leftTrimmed = (in1 * trimGainLinear);
			t_sample rightTrimmed = (in2 * trimGainLinear);
			m_leftInputDelay_14.write(in1);
			m_rightInputDelay_13.write(in2);
			m_leftOutputDelay_16.write(leftTrimmed);
			m_rightOutputDelay_15.write(rightTrimmed);
			t_sample mainSignalMono = ((rightTrimmed + leftTrimmed) * ((t_sample)0.707));
			t_sample smoothSidechainTrimGain = ((m_sidechainTrimHistory_63 * ((t_sample)0.999)) + (m_y_SCTRIM_79 * ((t_sample)0.001)));
			t_sample sidechainTrimHistoryNext = fixdenorm(smoothSidechainTrimGain);
			t_sample sidechainTrimGainLinear = dbtoa(smoothSidechainTrimGain);
			t_sample sidechainTrimReference = sidechainTrimGainLinear;
			t_sample leftSidechainTrimmed = (in3 * sidechainTrimReference);
			t_sample rightSidechainTrimmed = (in4 * sidechainTrimReference);
			t_sample expr_603 = leftSidechainTrimmed;
			t_sample expr_604 = rightSidechainTrimmed;
			t_sample sidechainSignalMono = ((rightSidechainTrimmed + leftSidechainTrimmed) * ((t_sample)0.707));
			t_sample mix_662 = (mainSignalMono + (smoothKeyMix * (sidechainSignalMono - mainSignalMono)));
			t_sample keyMixedSignal = mix_662;
			t_sample ONSIDECHAIN = smoothScEnable;
			t_sample REACT = smoothReact;
			t_sample EXTRA_SMOOTH = smoothSmoothAmount;
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
			t_sample lpfFromHistory1 = (m_lpfHistory_48 * lpfCoeffA1);
			t_sample lpfFromHistory2 = (m_lpfHistory_50 * lpfCoeffA0);
			t_sample lpfFeedback1 = (m_lpfHistory_49 * lpfCoeffB2);
			t_sample lpfFeedback2 = (m_lpfHistory_47 * lpfCoeffB1);
			t_sample lpfFilteredSignal = (((lpfFromHistory2 + lpfFromHistory1) + lpfInput) - (lpfFeedback2 + lpfFeedback1));
			t_sample lpfHistory1Next = fixdenorm(m_lpfHistory_48);
			t_sample lpfHistory2Next = fixdenorm(m_lpfHistory_47);
			t_sample lpfHistory3Next = fixdenorm(sidechainMixedSignal);
			t_sample lpfHistory4Next = fixdenorm(lpfFilteredSignal);
			t_sample lpfStage2Input = (lpfFilteredSignal * lpfCoeffA2);
			t_sample lpfStage2FromHistory1 = (m_lpfHistory_44 * lpfCoeffA1);
			t_sample lpfStage2FromHistory2 = (m_lpfHistory_46 * lpfCoeffA0);
			t_sample lpfStage2Feedback1 = (m_lpfHistory_45 * lpfCoeffB2);
			t_sample lpfStage2Feedback2 = (m_lpfHistory_43 * lpfCoeffB1);
			t_sample lpfStage2FilteredSignal = (((lpfStage2FromHistory2 + lpfStage2FromHistory1) + lpfStage2Input) - (lpfStage2Feedback2 + lpfStage2Feedback1));
			t_sample lpfHistory5Next = fixdenorm(m_lpfHistory_44);
			t_sample lpfHistory6Next = fixdenorm(m_lpfHistory_43);
			t_sample lpfHistory7Next = fixdenorm(lpfFilteredSignal);
			t_sample lpfHistory8Next = fixdenorm(lpfStage2FilteredSignal);
			t_sample lpfStage3Input = (lpfStage2FilteredSignal * lpfCoeffA2);
			t_sample lpfStage3FromHistory1 = (m_lpfHistory_32 * lpfCoeffA1);
			t_sample lpfStage3FromHistory2 = (m_lpfHistory_34 * lpfCoeffA0);
			t_sample lpfStage3Feedback1 = (m_lpfHistory_33 * lpfCoeffB2);
			t_sample lpfStage3Feedback2 = (m_lpfHistory_31 * lpfCoeffB1);
			t_sample lpfStage3FilteredSignal = (((lpfStage3FromHistory2 + lpfStage3FromHistory1) + lpfStage3Input) - (lpfStage3Feedback2 + lpfStage3Feedback1));
			t_sample lpfHistory9Next = fixdenorm(m_lpfHistory_32);
			t_sample lpfHistory10Next = fixdenorm(m_lpfHistory_31);
			t_sample lpfHistory11Next = fixdenorm(lpfStage2FilteredSignal);
			t_sample lpfHistory12Next = fixdenorm(lpfStage3FilteredSignal);
			t_sample lpfStage4Input = (lpfStage3FilteredSignal * lpfCoeffA2);
			t_sample lpfStage4FromHistory1 = (m_lpfHistory_28 * lpfCoeffA1);
			t_sample lpfStage4FromHistory2 = (m_lpfHistory_30 * lpfCoeffA0);
			t_sample lpfStage4Feedback1 = (m_lpfHistory_29 * lpfCoeffB2);
			t_sample lpfStage4Feedback2 = (m_lpfHistory_27 * lpfCoeffB1);
			t_sample lpfStage4FilteredSignal = (((lpfStage4FromHistory2 + lpfStage4FromHistory1) + lpfStage4Input) - (lpfStage4Feedback2 + lpfStage4Feedback1));
			t_sample lpfHistory13Next = fixdenorm(m_lpfHistory_28);
			t_sample lpfHistory14Next = fixdenorm(m_lpfHistory_27);
			t_sample lpfHistory15Next = fixdenorm(lpfStage3FilteredSignal);
			t_sample lpfHistory16Next = fixdenorm(lpfStage4FilteredSignal);
			if (lpfOrderDiscrete == ((int)0)) {
				lpfFinalSignal = lpfFilteredSignal;
				
			} else {
				lpfFinalSignal = lpfStage2FilteredSignal;
				
			};
			t_sample mix_663 = (sidechainMixedSignal + (ONSIDECHAIN * (lpfFinalSignal - sidechainMixedSignal)));
			t_sample lpfMixedSignal = mix_663;
			t_sample hpfInput = (lpfMixedSignal * hpfCoeffA2);
			t_sample hpfFromHistory1 = (m_hpfHistory_40 * hpfCoeffA1);
			t_sample hpfFromHistory2 = (m_hpfHistory_41 * hpfCoeffA0);
			t_sample hpfFeedback1 = (m_hpfHistory_42 * hpfCoeffB2);
			t_sample hpfFeedback2 = (m_hpfHistory_39 * hpfCoeffB1);
			t_sample hpfFilteredSignal = (((hpfFromHistory2 + hpfFromHistory1) + hpfInput) - (hpfFeedback2 + hpfFeedback1));
			t_sample hpfHistory1Next = fixdenorm(m_hpfHistory_39);
			t_sample hpfHistory2Next = fixdenorm(m_hpfHistory_40);
			t_sample hpfHistory3Next = fixdenorm(lpfMixedSignal);
			t_sample hpfHistory4Next = fixdenorm(hpfFilteredSignal);
			t_sample hpfStage2Input = (hpfFilteredSignal * hpfCoeffA2);
			t_sample hpfStage2FromHistory1 = (m_hpfHistory_36 * hpfCoeffA1);
			t_sample hpfStage2FromHistory2 = (m_hpfHistory_37 * hpfCoeffA0);
			t_sample hpfStage2Feedback1 = (m_hpfHistory_38 * hpfCoeffB2);
			t_sample hpfStage2Feedback2 = (m_hpfHistory_35 * hpfCoeffB1);
			t_sample hpfStage2FilteredSignal = (((hpfStage2FromHistory2 + hpfStage2FromHistory1) + hpfStage2Input) - (hpfStage2Feedback2 + hpfStage2Feedback1));
			t_sample hpfHistory5Next = fixdenorm(m_hpfHistory_35);
			t_sample hpfHistory6Next = fixdenorm(m_hpfHistory_36);
			t_sample hpfHistory7Next = fixdenorm(hpfFilteredSignal);
			t_sample hpfHistory8Next = fixdenorm(hpfStage2FilteredSignal);
			t_sample hpfStage3Input = (hpfStage2FilteredSignal * hpfCoeffA2);
			t_sample hpfStage3FromHistory1 = (m_hpfHistory_24 * hpfCoeffA1);
			t_sample hpfStage3FromHistory2 = (m_hpfHistory_25 * hpfCoeffA0);
			t_sample hpfStage3Feedback1 = (m_hpfHistory_26 * hpfCoeffB2);
			t_sample hpfStage3Feedback2 = (m_hpfHistory_23 * hpfCoeffB1);
			t_sample hpfStage3FilteredSignal = (((hpfStage3FromHistory2 + hpfStage3FromHistory1) + hpfStage3Input) - (hpfStage3Feedback2 + hpfStage3Feedback1));
			t_sample hpfHistory9Next = fixdenorm(m_hpfHistory_23);
			t_sample hpfHistory10Next = fixdenorm(m_hpfHistory_24);
			t_sample hpfHistory11Next = fixdenorm(hpfStage2FilteredSignal);
			t_sample hpfHistory12Next = fixdenorm(hpfStage3FilteredSignal);
			t_sample hpfStage4Input = (hpfStage3FilteredSignal * hpfCoeffA2);
			t_sample hpfStage4FromHistory1 = (m_hpfHistory_20 * hpfCoeffA1);
			t_sample hpfStage4FromHistory2 = (m_hpfHistory_21 * hpfCoeffA0);
			t_sample hpfStage4Feedback1 = (m_hpfHistory_22 * hpfCoeffB2);
			t_sample hpfStage4Feedback2 = (m_hpfHistory_19 * hpfCoeffB1);
			t_sample hpfStage4FilteredSignal = (((hpfStage4FromHistory2 + hpfStage4FromHistory1) + hpfStage4Input) - (hpfStage4Feedback2 + hpfStage4Feedback1));
			t_sample hpfHistory13Next = fixdenorm(m_hpfHistory_19);
			t_sample hpfHistory14Next = fixdenorm(m_hpfHistory_20);
			t_sample hpfHistory15Next = fixdenorm(hpfStage3FilteredSignal);
			t_sample hpfHistory16Next = fixdenorm(hpfStage4FilteredSignal);
			if (hpfOrderDiscrete == ((int)0)) {
				hpfFinalSignal = hpfFilteredSignal;
				
			} else {
				hpfFinalSignal = hpfStage2FilteredSignal;
				
			};
			t_sample mix_664 = (lpfMixedSignal + (ONSIDECHAIN * (hpfFinalSignal - lpfMixedSignal)));
			t_sample sidechainProcessedSignal = mix_664;
			t_sample sidechainDetectionSignal = sidechainProcessedSignal;
			t_sample attack_ms = ((((t_sample)0.05) < smoothAttack) ? smoothAttack : ((t_sample)0.05));
			t_sample release_ms = ((((t_sample)0.1) < smoothRelease) ? smoothRelease : ((t_sample)0.1));
			t_sample attack_coeff = exp(safediv(((int)-1), ((attack_ms * samplerate) * ((t_sample)0.001))));
			t_sample release_coeff = exp(safediv(((int)-1), ((release_ms * samplerate) * ((t_sample)0.001))));
			t_sample input_squared = (sidechainDetectionSignal * sidechainDetectionSignal);
			t_sample oldest_sq = m_rmsDelay_10.read_step(rms_window);
			t_sample rms_sum_new = ((input_squared + m_rmsSum_9) - oldest_sq);
			t_sample rms_sum_clipped = ((((int)0) < rms_sum_new) ? rms_sum_new : ((int)0));
			t_sample rms_value_sharp = sqrt((rms_sum_clipped * rms_window_inv));
			m_rmsSum_9 = rms_sum_clipped;
			m_rmsDelay_10.write(input_squared);
			m_peakEnvSharp_8 = fabs(sidechainDetectionSignal);
			t_sample mix_665 = (m_peakEnvSharp_8 + (REACT * (rms_value_sharp - m_peakEnvSharp_8)));
			t_sample env_mix_sharp = mix_665;
			t_sample release_env_175 = (m_releaseHistSharp_7 * release_coeff);
			t_sample env_post_release_176 = ((env_mix_sharp < release_env_175) ? release_env_175 : env_mix_sharp);
			t_sample hold_env_sharp = env_post_release_176;
			if ((env_post_release_176 > m_held_peak_sharp_2)) {
				m_held_peak_sharp_2 = env_post_release_176;
				m_hold_counter_sharp_1 = ((int)0);
				hold_env_sharp = m_held_peak_sharp_2;
				
			} else {
				t_sample mina_97 = (m_hold_counter_sharp_1 + ((int)1));
				m_hold_counter_sharp_1 = ((hold_samples < mina_97) ? hold_samples : mina_97);
				if ((m_hold_counter_sharp_1 < hold_samples)) {
					hold_env_sharp = m_held_peak_sharp_2;
					env_post_release_176 = m_held_peak_sharp_2;
					
				} else {
					hold_env_sharp = env_post_release_176;
					m_held_peak_sharp_2 = env_post_release_176;
					
				};
				
			};
			t_sample attack_env_177 = (m_attackHistSharp_6 * attack_coeff);
			t_sample attack_mix_178 = (((((int)1) - attack_coeff) * hold_env_sharp) + (attack_coeff * attack_env_177));
			t_sample sharpEnvelopeFinal = ((attack_mix_178 < ((t_sample)1e-06)) ? ((t_sample)1e-06) : attack_mix_178);
			m_releaseHistSharp_7 = fixdenorm(env_post_release_176);
			m_attackHistSharp_6 = fixdenorm(attack_mix_178);
			t_sample smooth_amount_shaped = safepow(EXTRA_SMOOTH, ((t_sample)2.5));
			t_sample mix_666 = (((t_sample)0.001) + (smooth_amount_shaped * ((t_sample)99.999)));
			t_sample smooth_time_ms = mix_666;
			t_sample extra_smooth_coeff = exp(safediv(((int)-1), ((smooth_time_ms * samplerate) * ((t_sample)0.001))));
			t_sample expr_algo0 = atodb(sharpEnvelopeFinal);
			if ((EXTRA_SMOOTH >= ((t_sample)0.001))) {
				t_sample extraSmoothedSharpEnv = ((m_extraSmoothHistSharp_5 * extra_smooth_coeff) + (sharpEnvelopeFinal * (((int)1) - extra_smooth_coeff)));
				m_extraSmoothHistSharp_5 = fixdenorm(extraSmoothedSharpEnv);
				expr_algo0 = atodb(((extraSmoothedSharpEnv < ((t_sample)1e-06)) ? ((t_sample)1e-06) : extraSmoothedSharpEnv));
				
			};
			t_sample envelopeDetectorOutput = expr_algo0;
			t_sample finalEnvelopeDb = envelopeDetectorOutput;
			t_sample trimCompensationFactor = safediv(((int)1), trimGainLinear);
			t_sample sidechainTrimCompensated = (sidechainDetectionSignal * trimCompensationFactor);
			t_sample expansionRatio = smoothRatio;
			t_sample dynamicKneeWidth = smoothKneeExp;
			t_sample expansionGainReduction = softkneeSecondOrderExpansor_d_d_d_d(finalEnvelopeDb, smoothThresholdDb, expansionRatio, dynamicKneeWidth);
			t_sample gainReductionDb = (expansionGainReduction - finalEnvelopeDb);
			t_sample gainReductionDbLimited = ((gainReductionDb < smoothRange) ? smoothRange : gainReductionDb);
			t_sample gainReductionLinear = dbtoa(gainReductionDbLimited);
			t_sample expandedLeft = (leftDelayedForCompression * gainReductionLinear);
			t_sample delta_control = ((smoothDeltaMode <= ((int)0)) ? ((int)0) : ((smoothDeltaMode >= ((int)1)) ? ((int)1) : smoothDeltaMode));
			t_sample delta_signal = (leftDelayedForCompression - expandedLeft);
			t_sample mix_668 = (expandedLeft + (delta_control * (delta_signal - expandedLeft)));
			t_sample autoMakeupGainReduction = gainReductionDb;
			t_sample reduction_smooth_mul = (m_reductionHistLeft_4 * ((t_sample)0.999));
			t_sample reduction_current_mul = (autoMakeupGainReduction * ((t_sample)0.001));
			t_sample reduction_smoothed = (reduction_current_mul + reduction_smooth_mul);
			t_sample mix_669 = (smoothMakeupGain + (smoothDeltaMode * (((int)0) - smoothMakeupGain)));
			t_sample makeup_linear = dbtoa(mix_669);
			t_sample leftWithMakeup = (mix_668 * makeup_linear);
			m_reductionHistLeft_4 = fixdenorm(reduction_smoothed);
			t_sample mix_670 = (leftDelayedForMixing + (mix_661 * (leftWithMakeup - leftDelayedForMixing)));
			t_sample mix_671 = (mix_670 + (smoothSoloSidechain * (sidechainDelayedWrite - mix_670)));
			t_sample leftWithSidechain = mix_671;
			t_sample saturationAmount = ((smoothSoftclip <= ((int)0)) ? ((int)0) : ((smoothSoftclip >= ((int)1)) ? ((int)1) : smoothSoftclip));
			t_sample mix_672 = (saturationAmount + (smoothDeltaMode * (((int)0) - saturationAmount)));
			t_sample leftSaturated = ((int)0);
			if ((leftWithSidechain > ((int)0))) {
				leftSaturated = (tanh((leftWithSidechain * ((t_sample)1.2))) * ((t_sample)0.833));
				
			} else {
				leftSaturated = (tanh((leftWithSidechain * ((t_sample)0.8))) * ((t_sample)1.25));
				
			};
			t_sample leftSaturated_592 = ((leftSaturated <= ((t_sample)-0.989)) ? ((t_sample)-0.989) : ((leftSaturated >= ((t_sample)0.989)) ? ((t_sample)0.989) : leftSaturated));
			t_sample mix_673 = (leftWithSidechain + (mix_672 * (leftSaturated_592 - leftWithSidechain)));
			t_sample mix_674 = (leftDelayedForMixing + (smoothBypassAmount * (mix_673 - leftDelayedForMixing)));
			t_sample rightGainReductionDb = gainReductionDbLimited;
			t_sample expandedRight = (rightDelayedForCompression * gainReductionLinear);
			t_sample rightDeltaControl = ((smoothDeltaMode <= ((int)0)) ? ((int)0) : ((smoothDeltaMode >= ((int)1)) ? ((int)1) : smoothDeltaMode));
			t_sample rightDeltaSignal = (rightDelayedForCompression - expandedRight);
			t_sample mix_675 = (expandedRight + (rightDeltaControl * (rightDeltaSignal - expandedRight)));
			t_sample rightAutoMakeupGainReduction = rightGainReductionDb;
			t_sample rightReductionSmoothMul = (m_reductionHistRight_3 * ((t_sample)0.999));
			t_sample rightReductionCurrentMul = (rightAutoMakeupGainReduction * ((t_sample)0.001));
			t_sample rightReductionSmoothed = (rightReductionCurrentMul + rightReductionSmoothMul);
			t_sample mix_676 = (smoothMakeupGain + (smoothDeltaMode * (((int)0) - smoothMakeupGain)));
			t_sample rightMakeupLinear = dbtoa(mix_676);
			t_sample rightWithMakeup = (mix_675 * rightMakeupLinear);
			m_reductionHistRight_3 = fixdenorm(rightReductionSmoothed);
			t_sample mix_677 = (rightDelayedForMixing + (mix_661 * (rightWithMakeup - rightDelayedForMixing)));
			t_sample mix_678 = (mix_677 + (smoothSoloSidechain * (sidechainDelayedTap - mix_677)));
			t_sample rightWithSidechain = mix_678;
			t_sample rightSaturationAmount = ((smoothSoftclip <= ((int)0)) ? ((int)0) : ((smoothSoftclip >= ((int)1)) ? ((int)1) : smoothSoftclip));
			t_sample mix_679 = (rightSaturationAmount + (smoothDeltaMode * (((int)0) - rightSaturationAmount)));
			t_sample rightSaturated = ((int)0);
			if ((rightWithSidechain > ((int)0))) {
				rightSaturated = (tanh((rightWithSidechain * ((t_sample)1.2))) * ((t_sample)0.833));
				
			} else {
				rightSaturated = (tanh((rightWithSidechain * ((t_sample)0.8))) * ((t_sample)1.25));
				
			};
			t_sample rightSaturated_594 = ((rightSaturated <= ((t_sample)-0.989)) ? ((t_sample)-0.989) : ((rightSaturated >= ((t_sample)0.989)) ? ((t_sample)0.989) : rightSaturated));
			t_sample mix_680 = (rightWithSidechain + (mix_679 * (rightSaturated_594 - rightWithSidechain)));
			t_sample mix_681 = (rightDelayedForMixing + (smoothBypassAmount * (mix_680 - rightDelayedForMixing)));
			t_sample realExpansionLinear = dbtoa(gainReductionDbLimited);
			t_sample mix_682 = (((int)1) + (bypassInverted * (realExpansionLinear - ((int)1))));
			t_sample gainReductionOutput = ((((int)1) < mix_682) ? ((int)1) : mix_682);
			m_deltaModeHistory_71 = deltaModeHistoryNext;
			m_thresholdHistory_70 = thresholdHistoryNext;
			m_makeupGainHistory_69 = makeupGainHistoryNext;
			m_soloSidechainHistory_68 = soloSidechainHistoryNext;
			m_bypassAmountHistory_67 = bypassAmountHistoryNext;
			m_keyMixHistory_66 = keyMixHistoryNext;
			m_dryWetMixHistory_65 = dryWetMixHistoryNext;
			m_trimHistory_64 = trimHistoryNext;
			m_sidechainTrimHistory_63 = sidechainTrimHistoryNext;
			m_ratioHistory_62 = ratioHistoryNext;
			m_rangeHistory_61 = rangeHistoryNext;
			m_kneeExpHistory_60 = kneeExpHistoryNext;
			m_reactHistory_59 = reactHistoryNext;
			m_smoothAmountHistory_58 = smoothAmountHistoryNext;
			m_lookaheadHistory_57 = lookaheadHistoryNext;
			m_hpfFreqHistory_56 = hpfFreqHistoryNext;
			m_lpfFreqHistory_55 = lpfFreqHistoryNext;
			m_scEnableHistory_54 = scEnableHistoryNext;
			m_softclipHistory_53 = softclipHistoryNext;
			m_atkHistory_52 = atkHistoryNext;
			m_relHistory_51 = relHistoryNext;
			m_lpfHistory_50 = lpfHistory1Next;
			m_lpfHistory_49 = lpfHistory2Next;
			m_lpfHistory_48 = lpfHistory3Next;
			m_lpfHistory_47 = lpfHistory4Next;
			m_lpfHistory_46 = lpfHistory5Next;
			m_lpfHistory_45 = lpfHistory6Next;
			m_lpfHistory_44 = lpfHistory7Next;
			m_lpfHistory_43 = lpfHistory8Next;
			m_lpfHistory_34 = lpfHistory9Next;
			m_lpfHistory_33 = lpfHistory10Next;
			m_lpfHistory_32 = lpfHistory11Next;
			m_lpfHistory_31 = lpfHistory12Next;
			m_lpfHistory_30 = lpfHistory13Next;
			m_lpfHistory_29 = lpfHistory14Next;
			m_lpfHistory_28 = lpfHistory15Next;
			m_lpfHistory_27 = lpfHistory16Next;
			m_hpfHistory_42 = hpfHistory1Next;
			m_hpfHistory_41 = hpfHistory2Next;
			m_hpfHistory_40 = hpfHistory3Next;
			m_hpfHistory_39 = hpfHistory4Next;
			m_hpfHistory_38 = hpfHistory5Next;
			m_hpfHistory_37 = hpfHistory6Next;
			m_hpfHistory_36 = hpfHistory7Next;
			m_hpfHistory_35 = hpfHistory8Next;
			m_hpfHistory_26 = hpfHistory9Next;
			m_hpfHistory_25 = hpfHistory10Next;
			m_hpfHistory_24 = hpfHistory11Next;
			m_hpfHistory_23 = hpfHistory12Next;
			m_hpfHistory_22 = hpfHistory13Next;
			m_hpfHistory_21 = hpfHistory14Next;
			m_hpfHistory_20 = hpfHistory15Next;
			m_hpfHistory_19 = hpfHistory16Next;
			m_hpfOrderHistory_18 = hpfOrderHistoryNext;
			m_lpfOrderHistory_17 = lpfOrderHistoryNext;
			m_sidechainWriteDelay_11.write(sidechainTrimCompensated);
			m_sidechainTapDelay_12.write(sidechainTrimCompensated);
			t_sample out5 = rightDelayedForCompression;
			t_sample out1 = mix_674;
			t_sample out6 = expr_603;
			t_sample out4 = leftDelayedForCompression;
			t_sample out2 = mix_681;
			t_sample out7 = expr_604;
			t_sample out3 = gainReductionOutput;
			m_rmsDelay_10.step();
			m_sidechainWriteDelay_11.step();
			m_sidechainTapDelay_12.step();
			m_rightInputDelay_13.step();
			m_leftInputDelay_14.step();
			m_rightOutputDelay_15.step();
			m_leftOutputDelay_16.step();
			// assign results to output buffer;
			(*(__out1++)) = out1;
			(*(__out2++)) = out2;
			(*(__out3++)) = out3;
			(*(__out4++)) = out4;
			(*(__out5++)) = out5;
			(*(__out6++)) = out6;
			(*(__out7++)) = out7;
			
		};
		return __exception;
		
	};
	inline void set_p_BYPASS(t_param _value) {
		m_p_BYPASS_72 = (_value < 0 ? 0 : (_value > 1 ? 1 : _value));
	};
	inline void set_u_SOFTCLIP(t_param _value) {
		m_u_SOFTCLIP_73 = (_value < 0 ? 0 : (_value > 1 ? 1 : _value));
	};
	inline void set_n_LOOKAHEAD(t_param _value) {
		m_n_LOOKAHEAD_74 = (_value < 0 ? 0 : (_value > 10 ? 10 : _value));
	};
	inline void set_v_DELTA(t_param _value) {
		m_v_DELTA_75 = (_value < 0 ? 0 : (_value > 1 ? 1 : _value));
	};
	inline void set_o_DRYWET(t_param _value) {
		m_o_DRYWET_76 = (_value < 0 ? 0 : (_value > 1 ? 1 : _value));
	};
	inline void set_m_SOLOSC(t_param _value) {
		m_m_SOLOSC_77 = (_value < 0 ? 0 : (_value > 1 ? 1 : _value));
	};
	inline void set_r_KEY(t_param _value) {
		m_r_KEY_78 = (_value < 0 ? 0 : (_value > 1 ? 1 : _value));
	};
	inline void set_y_SCTRIM(t_param _value) {
		m_y_SCTRIM_79 = (_value < -12 ? -12 : (_value > 12 ? 12 : _value));
	};
	inline void set_l_SC(t_param _value) {
		m_l_SC_80 = (_value < 0 ? 0 : (_value > 1 ? 1 : _value));
	};
	inline void set_k_LPFORDER(t_param _value) {
		m_k_LPFORDER_81 = (_value < 0 ? 0 : (_value > 1 ? 1 : _value));
	};
	inline void set_k_LPF(t_param _value) {
		m_k_LPF_82 = (_value < 20 ? 20 : (_value > 20000 ? 20000 : _value));
	};
	inline void set_j_HPFORDER(t_param _value) {
		m_j_HPFORDER_83 = (_value < 0 ? 0 : (_value > 1 ? 1 : _value));
	};
	inline void set_j_HPF(t_param _value) {
		m_j_HPF_84 = (_value < 20 ? 20 : (_value > 20000 ? 20000 : _value));
	};
	inline void set_z_SMOOTH(t_param _value) {
		m_z_SMOOTH_85 = (_value < 0 ? 0 : (_value > 1 ? 1 : _value));
	};
	inline void set_g_REACT(t_param _value) {
		m_g_REACT_86 = (_value < 0 ? 0 : (_value > 1 ? 1 : _value));
	};
	inline void set_q_KNEE(t_param _value) {
		m_q_KNEE_87 = (_value < 1 ? 1 : (_value > 20 ? 20 : _value));
	};
	inline void set_h_RANGE(t_param _value) {
		m_h_RANGE_88 = (_value < -100 ? -100 : (_value > 0 ? 0 : _value));
	};
	inline void set_i_MAKEUP(t_param _value) {
		m_i_MAKEUP_89 = (_value < -12 ? -12 : (_value > 12 ? 12 : _value));
	};
	inline void set_f_HOLD(t_param _value) {
		m_f_HOLD_90 = (_value < 0 ? 0 : (_value > 250 ? 250 : _value));
	};
	inline void set_e_REL(t_param _value) {
		m_e_REL_91 = (_value < 0.1 ? 0.1 : (_value > 1000 ? 1000 : _value));
	};
	inline void set_d_ATK(t_param _value) {
		m_d_ATK_92 = (_value < 0.1 ? 0.1 : (_value > 250 ? 250 : _value));
	};
	inline void set_c_RATIO(t_param _value) {
		m_c_RATIO_93 = (_value < 1 ? 1 : (_value > 40 ? 40 : _value));
	};
	inline void set_b_THD(t_param _value) {
		m_b_THD_94 = (_value < -60 ? -60 : (_value > 0 ? 0 : _value));
	};
	inline void set_a_TRIM(t_param _value) {
		m_a_TRIM_95 = (_value < -12 ? -12 : (_value > 12 ? 12 : _value));
	};
	inline t_sample softkneeSecondOrderExpansor_d_d_d_d(t_sample xg, t_sample T, t_sample R, t_sample W) {
		t_sample _softkneeSecondOrderExpansor_ret_1 = ((int)0);
		if ((R <= ((int)6))) {
			t_sample v_98 = ((R - ((int)1)) * ((t_sample)0.2));
			t_sample subtle_factor = ((v_98 <= ((int)0)) ? ((int)0) : ((v_98 >= ((int)1)) ? ((int)1) : v_98));
			t_sample gentle_expansion = (safepow(subtle_factor, ((t_sample)2.5)) * ((t_sample)0.5));
			t_sample normal_expansion = ((int)0);
			if (((((int)2) * (xg - T)) > W)) {
				normal_expansion = xg;
				
			} else {
				if (((((int)2) * fabs((xg - T))) > ((-((int)1)) * W))) {
					normal_expansion = (xg - safediv((safepow(((xg - T) - (W * ((t_sample)0.5))), ((int)2)) * (R - ((int)1))), (((int)2) * W)));
					
				} else {
					if (((((int)2) * (xg - T)) <= ((-((int)1)) * W))) {
						normal_expansion = (T + safediv((xg - T), R));
						
					};
					
				};
				
			};
			t_sample mix_667 = (xg + (gentle_expansion * (normal_expansion - xg)));
			_softkneeSecondOrderExpansor_ret_1 = mix_667;
			
		} else {
			if (((((int)2) * (xg - T)) > W)) {
				_softkneeSecondOrderExpansor_ret_1 = xg;
				
			} else {
				if (((((int)2) * fabs((xg - T))) > ((-((int)1)) * W))) {
					_softkneeSecondOrderExpansor_ret_1 = (xg - safediv((safepow(((xg - T) - (W * ((t_sample)0.5))), ((int)2)) * (R - ((int)1))), (((int)2) * W)));
					
				} else {
					if (((((int)2) * (xg - T)) <= ((-((int)1)) * W))) {
						_softkneeSecondOrderExpansor_ret_1 = (T + safediv((xg - T), R));
						
					};
					
				};
				
			};
			
		};
		return _softkneeSecondOrderExpansor_ret_1;
		
	};
	
} State;


///
///	Configuration for the genlib API
///

/// Number of signal inputs and outputs

int gen_kernel_numins = 4;
int gen_kernel_numouts = 7;

int num_inputs() { return gen_kernel_numins; }
int num_outputs() { return gen_kernel_numouts; }
int num_params() { return 24; }

/// Assistive lables for the signal inputs and outputs

const char *gen_kernel_innames[] = { "in1", "in2", "in3", "in4" };
const char *gen_kernel_outnames[] = { "out1", "out2", "out3", "out4", "out5", "out6", "out7" };

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
		case 1: self->set_b_THD(value); break;
		case 2: self->set_c_RATIO(value); break;
		case 3: self->set_d_ATK(value); break;
		case 4: self->set_e_REL(value); break;
		case 5: self->set_f_HOLD(value); break;
		case 6: self->set_g_REACT(value); break;
		case 7: self->set_h_RANGE(value); break;
		case 8: self->set_i_MAKEUP(value); break;
		case 9: self->set_j_HPF(value); break;
		case 10: self->set_j_HPFORDER(value); break;
		case 11: self->set_k_LPF(value); break;
		case 12: self->set_k_LPFORDER(value); break;
		case 13: self->set_l_SC(value); break;
		case 14: self->set_m_SOLOSC(value); break;
		case 15: self->set_n_LOOKAHEAD(value); break;
		case 16: self->set_o_DRYWET(value); break;
		case 17: self->set_p_BYPASS(value); break;
		case 18: self->set_q_KNEE(value); break;
		case 19: self->set_r_KEY(value); break;
		case 20: self->set_u_SOFTCLIP(value); break;
		case 21: self->set_v_DELTA(value); break;
		case 22: self->set_y_SCTRIM(value); break;
		case 23: self->set_z_SMOOTH(value); break;
		
		default: break;
	}
}

/// Get the value of a parameter of a State object

void getparameter(CommonState *cself, long index, t_param *value) {
	State *self = (State *)cself;
	switch (index) {
		case 0: *value = self->m_a_TRIM_95; break;
		case 1: *value = self->m_b_THD_94; break;
		case 2: *value = self->m_c_RATIO_93; break;
		case 3: *value = self->m_d_ATK_92; break;
		case 4: *value = self->m_e_REL_91; break;
		case 5: *value = self->m_f_HOLD_90; break;
		case 6: *value = self->m_g_REACT_86; break;
		case 7: *value = self->m_h_RANGE_88; break;
		case 8: *value = self->m_i_MAKEUP_89; break;
		case 9: *value = self->m_j_HPF_84; break;
		case 10: *value = self->m_j_HPFORDER_83; break;
		case 11: *value = self->m_k_LPF_82; break;
		case 12: *value = self->m_k_LPFORDER_81; break;
		case 13: *value = self->m_l_SC_80; break;
		case 14: *value = self->m_m_SOLOSC_77; break;
		case 15: *value = self->m_n_LOOKAHEAD_74; break;
		case 16: *value = self->m_o_DRYWET_76; break;
		case 17: *value = self->m_p_BYPASS_72; break;
		case 18: *value = self->m_q_KNEE_87; break;
		case 19: *value = self->m_r_KEY_78; break;
		case 20: *value = self->m_u_SOFTCLIP_73; break;
		case 21: *value = self->m_v_DELTA_75; break;
		case 22: *value = self->m_y_SCTRIM_79; break;
		case 23: *value = self->m_z_SMOOTH_85; break;
		
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
	self->__commonstate.params = (ParamInfo *)genlib_sysmem_newptr(24 * sizeof(ParamInfo));
	self->__commonstate.numparams = 24;
	// initialize parameter 0 ("m_a_TRIM_95")
	pi = self->__commonstate.params + 0;
	pi->name = "a_TRIM";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_a_TRIM_95;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = -12;
	pi->outputmax = 12;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 1 ("m_b_THD_94")
	pi = self->__commonstate.params + 1;
	pi->name = "b_THD";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_b_THD_94;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = -60;
	pi->outputmax = 0;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 2 ("m_c_RATIO_93")
	pi = self->__commonstate.params + 2;
	pi->name = "c_RATIO";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_c_RATIO_93;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 1;
	pi->outputmax = 40;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 3 ("m_d_ATK_92")
	pi = self->__commonstate.params + 3;
	pi->name = "d_ATK";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_d_ATK_92;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0.1;
	pi->outputmax = 250;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 4 ("m_e_REL_91")
	pi = self->__commonstate.params + 4;
	pi->name = "e_REL";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_e_REL_91;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0.1;
	pi->outputmax = 1000;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 5 ("m_f_HOLD_90")
	pi = self->__commonstate.params + 5;
	pi->name = "f_HOLD";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_f_HOLD_90;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 250;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 6 ("m_g_REACT_86")
	pi = self->__commonstate.params + 6;
	pi->name = "g_REACT";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_g_REACT_86;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 1;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 7 ("m_h_RANGE_88")
	pi = self->__commonstate.params + 7;
	pi->name = "h_RANGE";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_h_RANGE_88;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = -100;
	pi->outputmax = 0;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 8 ("m_i_MAKEUP_89")
	pi = self->__commonstate.params + 8;
	pi->name = "i_MAKEUP";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_i_MAKEUP_89;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = -12;
	pi->outputmax = 12;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 9 ("m_j_HPF_84")
	pi = self->__commonstate.params + 9;
	pi->name = "j_HPF";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_j_HPF_84;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 20;
	pi->outputmax = 20000;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 10 ("m_j_HPFORDER_83")
	pi = self->__commonstate.params + 10;
	pi->name = "j_HPFORDER";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_j_HPFORDER_83;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 1;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 11 ("m_k_LPF_82")
	pi = self->__commonstate.params + 11;
	pi->name = "k_LPF";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_k_LPF_82;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 20;
	pi->outputmax = 20000;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 12 ("m_k_LPFORDER_81")
	pi = self->__commonstate.params + 12;
	pi->name = "k_LPFORDER";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_k_LPFORDER_81;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 1;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 13 ("m_l_SC_80")
	pi = self->__commonstate.params + 13;
	pi->name = "l_SC";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_l_SC_80;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 1;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 14 ("m_m_SOLOSC_77")
	pi = self->__commonstate.params + 14;
	pi->name = "m_SOLOSC";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_m_SOLOSC_77;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 1;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 15 ("m_n_LOOKAHEAD_74")
	pi = self->__commonstate.params + 15;
	pi->name = "n_LOOKAHEAD";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_n_LOOKAHEAD_74;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 10;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 16 ("m_o_DRYWET_76")
	pi = self->__commonstate.params + 16;
	pi->name = "o_DRYWET";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_o_DRYWET_76;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 1;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 17 ("m_p_BYPASS_72")
	pi = self->__commonstate.params + 17;
	pi->name = "p_BYPASS";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_p_BYPASS_72;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 1;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 18 ("m_q_KNEE_87")
	pi = self->__commonstate.params + 18;
	pi->name = "q_KNEE";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_q_KNEE_87;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 1;
	pi->outputmax = 20;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 19 ("m_r_KEY_78")
	pi = self->__commonstate.params + 19;
	pi->name = "r_KEY";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_r_KEY_78;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 1;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 20 ("m_u_SOFTCLIP_73")
	pi = self->__commonstate.params + 20;
	pi->name = "u_SOFTCLIP";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_u_SOFTCLIP_73;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 1;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 21 ("m_v_DELTA_75")
	pi = self->__commonstate.params + 21;
	pi->name = "v_DELTA";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_v_DELTA_75;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = 0;
	pi->outputmax = 1;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 22 ("m_y_SCTRIM_79")
	pi = self->__commonstate.params + 22;
	pi->name = "y_SCTRIM";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_y_SCTRIM_79;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = true;
	pi->outputmin = -12;
	pi->outputmax = 12;
	pi->exp = 0;
	pi->units = "";		// no units defined
	// initialize parameter 23 ("m_z_SMOOTH_85")
	pi = self->__commonstate.params + 23;
	pi->name = "z_SMOOTH";
	pi->paramtype = GENLIB_PARAMTYPE_FLOAT;
	pi->defaultvalue = self->m_z_SMOOTH_85;
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


} // JCBExpander::
