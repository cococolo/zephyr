/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <nanokernel.h>
#include <arch/cpu.h>
#include <kernel_structs.h>
#include <misc/printk.h>
#include <inttypes.h>

const NANO_ESF _default_esf = {
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad,
	0xdeadbaad
};

/**
 *
 * @brief Nanokernel fatal error handler
 *
 * This routine is called when a fatal error condition is detected by either
 * hardware or software.
 *
 * The caller is expected to always provide a usable ESF.  In the event that the
 * fatal error does not have a hardware generated ESF, the caller should either
 * create its own or call _Fault instead.
 *
 * @param reason the reason that the handler was called
 * @param pEsf pointer to the exception stack frame
 *
 * @return This function does not return.
 */
FUNC_NORETURN void _NanoFatalErrorHandler(unsigned int reason,
					  const NANO_ESF *esf)
{
#ifdef CONFIG_PRINTK
	switch (reason) {
	case _NANO_ERR_CPU_EXCEPTION:
	case _NANO_ERR_SPURIOUS_INT:
		break;

	case _NANO_ERR_INVALID_TASK_EXIT:
		printk("***** Invalid Exit Software Error! *****\n");
		break;


	case _NANO_ERR_ALLOCATION_FAIL:
		printk("**** Kernel Allocation Failure! ****\n");
		break;

	default:
		printk("**** Unknown Fatal Error %u! ****\n", reason);
		break;
	}

	/* Subtract 4 from EA since we added 4 earlier so that the faulting
	 * instruction isn't retried.
	 *
	 * TODO: Only caller-saved registers get saved upon exception entry.
	 * We may want to introduce a config option to save and dump all
	 * registers, at the expense of some stack space.
	 */
	printk("Current thread ID: %p\n"
	       "Faulting instruction: 0x%" PRIx32 "\n"
	       "  r1: 0x%" PRIx32 "  r2: 0x%" PRIx32
	       "  r3: 0x%" PRIx32 "  r4: 0x%" PRIx32 "\n"
	       "  r5: 0x%" PRIx32 "  r6: 0x%" PRIx32
	       "  r7: 0x%" PRIx32 "  r8: 0x%" PRIx32 "\n"
	       "  r9: 0x%" PRIx32 " r10: 0x%" PRIx32
	       " r11: 0x%" PRIx32 " r12: 0x%" PRIx32 "\n"
	       " r13: 0x%" PRIx32 " r14: 0x%" PRIx32
	       " r15: 0x%" PRIx32 "  ra: 0x%" PRIx32 "\n"
	       "estatus: %" PRIx32 "\n", k_current_get(), esf->instr - 4,
	       esf->r1, esf->r2, esf->r3, esf->r4,
	       esf->r5, esf->r6, esf->r7, esf->r8,
	       esf->r9, esf->r10, esf->r11, esf->r12,
	       esf->r13, esf->r14, esf->r15, esf->ra,
	       esf->estatus);
#endif

	_SysFatalErrorHandler(reason, esf);
}

#if defined(CONFIG_EXTRA_EXCEPTION_INFO) && defined(CONFIG_PRINTK) \
	&& defined(ALT_CPU_HAS_EXTRA_EXCEPTION_INFO)
static char *cause_str(uint32_t cause_code)
{
	switch (cause_code) {
	case 0:
		return "reset";
	case 1:
		return "processor-only reset request";
	case 2:
		return "interrupt";
	case 3:
		return "trap";
	case 4:
		return "unimplemented instruction";
	case 5:
		return "illegal instruction";
	case 6:
		return "misaligned data address";
	case 7:
		return "misaligned destination address";
	case 8:
		return "division error";
	case 9:
		return "supervisor-only instruction address";
	case 10:
		return "supervisor-only instruction";
	case 11:
		return "supervisor-only data address";
	case 12:
		return "TLB miss";
	case 13:
		return "TLB permission violation (execute)";
	case 14:
		return "TLB permission violation (read)";
	case 15:
		return "TLB permission violation (write)";
	case 16:
		return "MPU region violation (instruction)";
	case 17:
		return "MPU region violation (data)";
	case 18:
		return "ECC TLB error";
	case 19:
		return "ECC fetch error (instruction)";
	case 20:
		return "ECC register file error";
	case 21:
		return "ECC data error";
	case 22:
		return "ECC data cache writeback error";
	case 23:
		return "bus instruction fetch error";
	case 24:
		return "bus data region violation";
	default:
		return "unknown";
	}
}
#endif

FUNC_NORETURN void _Fault(const NANO_ESF *esf)
{
#ifdef CONFIG_PRINTK
	/* Unfortunately, completely unavailable on Nios II/e cores */
#ifdef ALT_CPU_HAS_EXTRA_EXCEPTION_INFO
	uint32_t exc_reg, badaddr_reg, eccftl;
	enum nios2_exception_cause cause;

	exc_reg = _nios2_creg_read(NIOS2_CR_EXCEPTION);

	/* Bit 31 indicates potentially fatal ECC error */
	eccftl = (exc_reg & NIOS2_EXCEPTION_REG_ECCFTL_MASK) != 0;

	/* Bits 2-6 contain the cause code */
	cause = (exc_reg & NIOS2_EXCEPTION_REG_CAUSE_MASK)
		 >> NIOS2_EXCEPTION_REG_CAUSE_OFST;

	printk("Exception cause: %d ECCFTL: 0x%" PRIu32 "\n", cause, eccftl);
#if CONFIG_EXTRA_EXCEPTION_INFO
	printk("reason: %s\n", cause_str(cause));
#endif
	if (BIT(cause) & NIOS2_BADADDR_CAUSE_MASK) {
		badaddr_reg = _nios2_creg_read(NIOS2_CR_BADADDR);
		printk("Badaddr: 0x%" PRIx32 "\n", badaddr_reg);
	}
#endif /* ALT_CPU_HAS_EXTRA_EXCEPTION_INFO */
#endif /* CONFIG_PRINTK */

	_NanoFatalErrorHandler(_NANO_ERR_CPU_EXCEPTION, esf);
}


/**
 *
 * @brief Fatal error handler
 *
 * This routine implements the corrective action to be taken when the system
 * detects a fatal error.
 *
 * This sample implementation attempts to abort the current thread and allow
 * the system to continue executing, which may permit the system to continue
 * functioning with degraded capabilities.
 *
 * System designers may wish to enhance or substitute this sample
 * implementation to take other actions, such as logging error (or debug)
 * information to a persistent repository and/or rebooting the system.
 *
 * @param reason the fatal error reason
 * @param pEsf pointer to exception stack frame
 *
 * @return N/A
 */
FUNC_NORETURN void _SysFatalErrorHandler(unsigned int reason,
					 const NANO_ESF *pEsf)
{
	ARG_UNUSED(reason);
	ARG_UNUSED(pEsf);

	if (k_is_in_isr() || _is_thread_essential()) {
		printk("Fatal fault in %s! Spinning...\n",
		       k_is_in_isr() ? "ISR" : "essential thread");
#ifdef ALT_CPU_HAS_DEBUG_STUB
		_nios2_break();
#endif
		for (;;)
			; /* spin forever */
	}
	printk("Fatal fault in thread %p! Aborting.\n", _current);
	k_thread_abort(_current);

	CODE_UNREACHABLE;
}
