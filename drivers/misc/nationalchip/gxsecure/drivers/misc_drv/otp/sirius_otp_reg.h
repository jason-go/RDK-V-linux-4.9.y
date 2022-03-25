#ifndef __SIRIUS_OTP_REG_H__
#define __SIRIUS_OTP_REG_H__

/*****************************************************************
 * OTP_FLAG :
 * [  0]: ifcp testswitch flag
 * [  1]: rng test mode disable
 * [  2]: rng start mode selcet
 * [  3]: otp test disable
 * [  4]: scan mbist debug disable
 * [  5]: scpu secure boot enable
 * [  6]: acpu prevent development code flag
 * [  7]: ate interface disable
 * [  8]: a7 core disable
 * [  9]: CSA3 descrambler disable
 * [ 10]: CSA2 descrambler disable
 * [ 11]: aes descrambler disable
 * [ 12]: des descrambler disable
 * [ 13]: tdes descrambler disable
 * [ 14]: CSA1 cw 48bit mode descrambler disable
 * [ 15]: klm32 module disable
 * [ 16]: gen dynamic fun disable
 * [ 17]: gen klm fun disable
 * [ 18]: demux pes layer descrambler disable
 * [ 19]: demux psi data descrambler disable
 * [ 20]: irdeto key disable
 * [ 21]: H265 disable
 * [ 22]: H265 10bit disable
 * [ 23]: dvb acm disable
 * [ 24]: dvb t2mi disable
 * [ 25]: dvbc sel
 * [ 26]: ts1 in disable
 * [ 27]: gen kds key disable
 * [ 28]: ts buffer protect enable
 * [ 29]: acpu ree secure boot enable
 * [ 30]: es buffer protect enable
 * [ 31]: gp buffer protect enable
 * [ 32]: softer buffer protect enable[0]
 * [ 33]: video clear uncompressed buffer protect enable[0]
 * [ 34]: audio clear uncompressed buffer protect enable[0]
 * [ 35]: spdif output disable[0]
 * [ 36]: ts write function disable[0]
 * [ 37]: hdmi output disable[0]
 * [ 38]: ethernet disable[0]
 * [ 39]: audio uart disable[0]
 * [ 40]: uart1 disable[0]
 * [ 41]: uart2 disable[0]
 * [ 42]: ddr content scrambler enable[0]
 * [ 43]: ddr content access limit en[0]
 * [ 44]: secure boot level configure[0]
 * [ 45]: acpu ejtag debug mode configure[0]
 * [ 46]: acpu flash encrypt otp key mode enable[0]
 * [ 47]: scpu flash encrypt otp key mode enable[0]
 * [ 48]: acpu ejtag debug protect enable[0]
 * [ 49]: rcc enable[0]
 * [ 50]: acpu secure boot enable[0]
 * [ 51]: usb disable[0]
 * [ 52]: otp clk xtal to osc enable[0]
 * [ 53]: uart boot disable[0]
 * [ 54]: msr2 cw key ladder scheme fixed enable[0]
 * [ 55]: msr2 cw key ladder scheme configure[0]
 * [ 56]: scpu ts cw dual mode disable[0]
 * [ 57]: acpu ts cw dual mode disable[0]
 * [ 58]: scpu gp cw dual mode disable[0]
 * [ 59]: acpu gp cw dual mode disable[0]
 * [ 60]: scpu pvr dual mode disable[0]
 * [ 61]: ddr content scrambler dynamic key enable[0]
 * [ 62]: boot code enc enforcement enable[0]
 * [ 63]: image enc enforcement enable[0]
 * [ 64]: irdeto security jtag mode enable[0]
 * [ 65]: secure boot ca switch mode enable[0]
 * [ 66]: msr2 pvr key ladder scheme fixed enable[0]
 * [ 67]: msr2 pvr key ladder scheme configure[0]
 * [ 68]: scpu gen dynamic scr enable[0]
 * [ 69]: ifcp klm tdes disable[0]
 * [ 70]: acpu klm tdes disable[0]
 * [ 71]: rng stop signal enable[0]
 * [ 72]: scpu gen dynamic kcv enable[0]
 * [ 73]: arm dbgen disable[0]
 * [ 74]: arm niden disable[0]
 * [ 75]: arm spiden disable[0]
 * [ 76]: arm spniden disable[0]
 * [ 77]: acpu flash encrypt ext key enable[0]
 * [ 78]: scpu flash encrypt irdeto mode[0]
 * [ 79]: scpu gen dynamic get result disable[0]
 * [ 86]: sec0 lock[0]
 * [ 87]: sec1 lock[0]
 * [ 88]: sec2 lock[0]
 * [ 89]: sec3 lock[0]
 * [ 90]: sec4 lock[0]
 * [ 91]: sec5 lock[0]
 * [ 92]: sec8 lock[0]
 * [ 93]: sec9 lock[0]
 * [ 94]: id 1 lock[0]
 * [ 95]: id 2 lock[0]
 * [ 96]: rng start time control[0]
 * [ 97]: rng start time control[1]
 * [ 98]: rng start time control[2]
 * [ 99]: rng start time control[3]
 * [100]: pll configure select[0]
 * [101]: pll configure select[1]
 * [102]: pll configure select[2]
 * [103]: pll configure select[3]
 * [104]: gen klm cw1 stage select[0]
 * [105]: gen klm cw1 stage select[1]
 * [106]: gen klm cw1 stage select[2]
 * [107]: gen klm cw1 stage select[3]
 * [108]: gen klm cw2 stage select[0]
 * [109]: gen klm cw2 stage select[1]
 * [110]: gen klm cw2 stage select[2]
 * [111]: gen klm cw2 stage select[3]
 * [112]: gen klm pvr1 stage select[0]
 * [113]: gen klm pvr1 stage select[1]
 * [114]: gen klm pvr1 stage select[2]
 * [115]: gen klm pvr1 stage select[3]
 * [116]: gen klm pvr2 stage select[0]
 * [117]: gen klm pvr2 stage select[1]
 * [118]: gen klm pvr2 stage select[2]
 * [119]: gen klm pvr2 stage select[3]
 *
 **************************************************************/
#if defined (CPU_SCPU)

typedef union {
	unsigned int value;
	struct {
		unsigned address : 16;
		unsigned         : 14;
		unsigned rd      : 1;
		unsigned wr      : 1;
	} bits;
} rOTP_CTRL;

typedef union {
	unsigned int value;
	struct {
		unsigned           : 8;
		unsigned busy      : 1;
		unsigned rd_valid  : 1;
		unsigned init_done : 1;
		unsigned           : 1;
		unsigned rw_fail   : 1;
	} bits;
} rOTP_STATUS;

#else
typedef union {
	unsigned int value;
	struct {
		unsigned address : 16;
		unsigned wr_data : 8;
		unsigned         : 6;
		unsigned rd      : 1;
		unsigned wr      : 1;
	} bits;
} rOTP_CTRL;

typedef union {
	unsigned int value;
	struct {
		unsigned rd_data   : 8;
		unsigned busy      : 1;
		unsigned rd_valid  : 1;
		unsigned           : 2;
		unsigned rw_fail   : 1;
	} bits;
} rOTP_STATUS;
#endif

#if defined (CPU_SCPU)
typedef struct {
	unsigned int      reserved_0[20];
	rOTP_CTRL         otp_ctrl;
	rOTP_STATUS       otp_status;
	unsigned int      otp_write_data;
	unsigned int      otp_read_data;
	unsigned int      otp_flag[4];
} SiriusOTPReg;

#else

typedef struct {
	rOTP_CTRL         otp_ctrl;
	rOTP_STATUS       otp_status;
	unsigned int      otp_flag[4];
} SiriusOTPReg;
#endif

#endif
