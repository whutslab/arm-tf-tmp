/*
 * 
 * [31:31]  TYPE   1:Fast           0:Yielding
 * [30:30]  SMCCC  1:SMC64/HVC64    0:SMC32/HVC32
 * [29:24]  OEN    0x07000000 – 0x2F000000, pakms use number 7
 * [23:16]  REV    Must be zero (MBZ): 0x00000000
 * [15:0]   FID    Function id: 0x00000000-0x0000FFFF
 *
 */

#include <common/debug.h>
#include <common/runtime_svc.h>
#include <lib/cpus/errata_report.h>
#include <lib/smccc.h>
#include <arch_helpers.h>
#include <smccc_helpers.h>

/*
 * indexs of five 128-bit PA keys
 */
uint16_t apia_lo_index=0;
uint16_t apia_hi_index=3;
uint16_t apib_lo_index=7;
uint16_t apib_hi_index=4;
uint16_t apda_lo_index=1;
uint16_t apda_hi_index=6;
uint16_t apdb_lo_index=9;
uint16_t apdb_hi_index=2;
uint16_t apga_lo_index=5;
uint16_t apga_hi_index=8;
uint64_t master_keys[1024];
uint64_t Gn;

/*
 * uint32_t read_cntpct_el0(); 
 *
 */
static void init_master_keys(void)
{
	uint64_t tmp=read_cntpct_el0()^read_pmcr_el0();
	tmp=(tmp<<13)|read_cntpct_el0();
	tmp=(tmp<<13)|read_cntpct_el0();
	tmp=(tmp<<13)|read_cntpct_el0();
	NOTICE("Initialization nonce for pauth master_keys: 0x%llx\n",tmp);
	NOTICE("Generation nonce: 0x%llx\n",Gn);
	master_keys[apia_lo_index]=(0x2093b9ff55d71dd8)^tmp;
	tmp=(tmp<<32)|read_cntpct_el0();
	master_keys[apia_hi_index]=(0x06f8c17953eeeeab)^tmp;
	tmp=(tmp<<32)|read_cntpct_el0();
	master_keys[apib_lo_index]=(0x5ce85f627fc2ade7)^tmp;
	tmp=(tmp<<32)|read_cntpct_el0();
	master_keys[apib_hi_index]=(0x0f0cf9b710235099)^tmp;
	tmp=(tmp<<32)|read_cntpct_el0();
	master_keys[apda_lo_index]=(0x9d319ea29a2ffe12)^tmp;
	tmp=(tmp<<32)|read_cntpct_el0();
	master_keys[apda_hi_index]=(0x56fd34dc9d83d312)^tmp;
	tmp=(tmp<<32)|read_cntpct_el0();
	master_keys[apdb_lo_index]=(0x2fe592e2ebb7e26c)^tmp;
	tmp=(tmp<<32)|read_cntpct_el0();
	master_keys[apdb_hi_index]=(0xb2205dc7a8d26fb7)^tmp;
	tmp=(tmp<<32)|read_cntpct_el0();
	master_keys[apga_lo_index]=(0xc3c84b1adb44415c)^tmp;
	tmp=(tmp<<32)|read_cntpct_el0();
	master_keys[apga_hi_index]=(0x54997215a6d9223b)^tmp;
}
static int32_t pakms_svc_init(void)
{
	NOTICE("................pakms is initializing................\n");
	//enable pac*/aut* instructions in EL1 and EL3
	asm(
	"mrs x9,SCTLR_EL1;"
	"ldr x10,=0xc8002000;"
	"orr x9,x9,x10;"
	"msr SCTLR_EL1,x9;"
	"mrs x9,SCTLR_EL3;"
	"ldr x10,=0xc8002000;"
	"orr x9,x9,x10;"
	"msr SCTLR_EL3,x9;");
	//initial Gn,master_keys
	Gn=read_cntpct_el0()^read_pmcr_el0();
	init_master_keys();
	NOTICE("pauth_apia: 0x%llx%llx\n",master_keys[apia_lo_index],master_keys[apia_hi_index]);
	NOTICE("pauth_apib: 0x%llx%llx\n",master_keys[apib_lo_index],master_keys[apib_hi_index]);
	NOTICE("pauth_apda: 0x%llx%llx\n",master_keys[apda_lo_index],master_keys[apda_hi_index]);
	NOTICE("pauth_apdb: 0x%llx%llx\n",master_keys[apdb_lo_index],master_keys[apdb_hi_index]);
	NOTICE("pauth_apga: 0x%llx%llx\n",master_keys[apga_lo_index],master_keys[apga_hi_index]);
	return 0;
}

/*
 * Top-level Arm Architectural Service SMC handler.
 */
#define NOM_PAC_APIA_FID64 0xc700f10f
#define NOM_PAC_APIB_FID64 0xc700f20f
#define NOM_PAC_APDA_FID64 0xc700f40f
#define NOM_PAC_APDB_FID64 0xc700f80f
#define NOM_PAC_APGA_FID64 0xc700ff0f
#define NOM_AUT_APIA_FID64 0xc700f1f0
#define NOM_AUT_APIB_FID64 0xc700f2f0
#define NOM_AUT_APDA_FID64 0xc700f4f0
#define NOM_AUT_APDB_FID64 0xc700f8f0
#define NOM_AUT_APGA_FID64 0xc700fff0

#define SEC_PAC_APIA_FID64 0xc70001f0
#define SEC_PAC_APIB_FID64 0xc70002f0
#define SEC_PAC_APDA_FID64 0xc70004f0
#define SEC_PAC_APDB_FID64 0xc70008f0
#define SEC_PAC_APGA_FID64 0xc7000ff0
#define SEC_AUT_APIA_FID64 0xc700010f
#define SEC_AUT_APIB_FID64 0xc700020f
#define SEC_AUT_APDA_FID64 0xc700040f
#define SEC_AUT_APDB_FID64 0xc700080f
#define SEC_AUT_APGA_FID64 0xc7000f0f
uint64_t count=0;
static uintptr_t pakms_svc_smc_handler(uint32_t smc_fid,
	u_register_t x1,
	u_register_t x2,
	u_register_t x3,
	u_register_t x4,
	void *cookie,
	void *handle,
	u_register_t flags)
{
	/* Todo: 
	 * update Gn 
	 * change master_keys' location
	 */
	if(count%10==0)Gn=Gn^read_cntpct_el0();
	count++;
	switch (smc_fid) {
	case NOM_PAC_APIA_FID64:
		/* x1:fid */
		asm(
		"msr APGAKeyLo_EL1,%[Kmlo];"//derive pa keys for fid
		"msr APGAKeyHi_EL1,%[Kmhi];"
		"pacga x10,%[Gn],%[fid];"
		"lsr x9,x10,#32;"
		"pacga x10,%[fid],%[Gn];"
		"adc x10,x10,x9;"
		"msr APIAKeyLo_EL1,x10;"//derive APIAKeyLo_EL1 key for fid
		"msr APGAKeyLo_EL1,x10;"
		"pacga x10,%[Gn],%[fid];"
		"lsr x9,x10,#32;"
		"pacga x10,%[fid],%[Gn];"
		"adc x10,x10,x9;"
		"msr APIAKeyHi_EL1,x10;"//derive APIAKeyHi_EL1 key for fid
		"msr APGAKeyHi_EL1,x10;"
		:
		:[fid]"r"(x1),[Kmlo]"r"(master_keys[apia_lo_index]),[Kmhi]"r"(master_keys[apia_hi_index]),[Gn]"r"(Gn)
		:"cc","x9","x10"
		);
		/* return Gn */
		SMC_RET1(handle,Gn);
	case NOM_AUT_APIA_FID64:
		/* x1:fid x2:Gn */
		asm(
		"msr APGAKeyLo_EL1,%[Kmlo];"//derive pa keys for fid
		"msr APGAKeyHi_EL1,%[Kmhi];"
		"pacga x10,%[Gn],%[fid];"
		"lsr x9,x10,#32;"
		"pacga x10,%[fid],%[Gn];"
		"adc x10,x10,x9;"
		"msr APIAKeyLo_EL1,x10;"//derive APIAKeyLo_EL1 key for fid
		"msr APGAKeyLo_EL1,x10;"
		"pacga x10,%[Gn],%[fid];"
		"lsr x9,x10,#32;"
		"pacga x10,%[fid],%[Gn];"
		"adc x10,x10,x9;"
		"msr APIAKeyHi_EL1,x10;"//derive APIAKeyHi_EL1 key for fid
		"msr APGAKeyHi_EL1,x10;"
		:
		:[fid]"r"(x1),[Gn]"r"(x2),[Kmlo]"r"(master_keys[apia_lo_index]),[Kmhi]"r"(master_keys[apia_hi_index])
		:"cc","x9","x10"
		);
		/* return */
		SMC_RET0(handle);
	default:
		WARN("Unimplemented Arm Architecture Service Call: 0x%x \n",
			smc_fid);
		SMC_RET1(handle, SMC_UNK);
	}
}

/* Register Standard Service Calls as runtime service */
DECLARE_RT_SVC(
		pakms_svc,
		OEN_PAKMS_START,
		OEN_PAKMS_END,
		SMC_TYPE_FAST,
		pakms_svc_init,
		pakms_svc_smc_handler
);
