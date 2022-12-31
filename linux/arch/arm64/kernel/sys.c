/*
 * AArch64-specific system calls implementation
 *
 * Copyright (C) 2012 ARM Ltd.
 * Author: Catalin Marinas <catalin.marinas@arm.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/compiler.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/export.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/syscalls.h>

#include <asm/cpufeature.h>
#include <asm/syscall.h>

SYSCALL_DEFINE1(pakms_pac,unsigned long,fid)
{
	unsigned long ret=0;
//	unsigned long lo=0,hi=0;
//	printk("This is Syscall Function -- pakms_pac.\n");
//	printk("pakms: fid is 0x%lx \n", fid);
	asm(
		"ldr x0,=0xc700f10f;"
		"mov x1,%[fid];"
		"smc #0;"
		"mov %[Gn],x0;"
//		"mrs %[lo],APIAKeyLo_EL1;"
//		"mrs %[hi],APIAKeyHi_EL1;"
//		:[Gn]"+r"(ret),[lo]"+r"(lo),[hi]"+r"(hi)
		:[Gn]"+r"(ret)
		:[fid]"r"(fid)
		:"x0","x1"
	);
//	printk("el1 pac apia: 0x%lx%lx \n", hi,lo);
	return ret;
}
SYSCALL_DEFINE2(pakms_aut,unsigned long,fid,unsigned long,Gn)
{
	unsigned long ret=0;
//	unsigned long lo=0,hi=0;
//	printk("This is Syscall Function -- pakms_aut.\n");
//	printk("pakms: fid is 0x%lx \n", fid);
//	printk("pakms: Gn  is 0x%lx \n", Gn);
	asm(
		"ldr x0,=0xc700f1f0;"
		"mov x1,%[fid];"
		"mov x2,%[Gn];"
		"smc #0;"
//		"mrs %[lo],APIAKeyLo_EL1;"
//		"mrs %[hi],APIAKeyHi_EL1;"
//		:[lo]"+r"(lo),[hi]"+r"(hi)
		:
		:[fid]"r"(fid),[Gn]"r"(Gn)
		:"x0","x1","x2"
	);
//	printk("el1 aut apia: 0x%lx%lx \n", hi,lo);
	return ret;
}

SYSCALL_DEFINE6(mmap, unsigned long, addr, unsigned long, len,
		unsigned long, prot, unsigned long, flags,
		unsigned long, fd, off_t, off)
{
	if (offset_in_page(off) != 0)
		return -EINVAL;

	return ksys_mmap_pgoff(addr, len, prot, flags, fd, off >> PAGE_SHIFT);
}

SYSCALL_DEFINE1(arm64_personality, unsigned int, personality)
{
	if (personality(personality) == PER_LINUX32 &&
		!system_supports_32bit_el0())
		return -EINVAL;
	return ksys_personality(personality);
}

/*
 * Wrappers to pass the pt_regs argument.
 */
#define sys_personality		sys_arm64_personality

asmlinkage long sys_ni_syscall(const struct pt_regs *);
#define __arm64_sys_ni_syscall	sys_ni_syscall

#undef __SYSCALL
#define __SYSCALL(nr, sym)	asmlinkage long __arm64_##sym(const struct pt_regs *);
#include <asm/unistd.h>

#undef __SYSCALL
#define __SYSCALL(nr, sym)	[nr] = (syscall_fn_t)__arm64_##sym,

const syscall_fn_t sys_call_table[__NR_syscalls] = {
	[0 ... __NR_syscalls - 1] = (syscall_fn_t)sys_ni_syscall,
#include <asm/unistd.h>
};
