/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "InterruptManager.h"
#include <machine/Machine.h>
#include <machine/types.h>
#include <utilities/utility.h>
#include <processor/Processor.h>
#include <panic.h>
#ifdef DEBUGGER
  #include <Debugger.h>
#endif
#include <Log.h>

#define SYSCALL_INTERRUPT_NUMBER 8
#define BREAKPOINT_INTERRUPT_NUMBER 9

const char *g_ExceptionNames[32] = {
  "Interrupt",
  "TLB modification exception",
  "TLB exception (load or instruction fetch)",
  "TLB exception (store)",
  "Address error exception (load or instruction fetch)",
  "Address error exception (store)",
  "Bus error exception (instruction fetch)",
  "Bus error exception (data: load or store)",
  "Syscall exception",
  "Breakpoint exception",
  "Reserved instruction exception",
  "Coprocessor unusable exception",
  "Arithmetic overflow exception",
  "Trap exception",
  "LDCz/SDCz to uncached address",
  "Virtual coherency exception",
  "Machine check exception",
  "Floating point exception",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Watchpoint exception",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
};

ARMV7InterruptManager ARMV7InterruptManager::m_Instance;

SyscallManager &SyscallManager::instance()
{
  return ARMV7InterruptManager::instance();
}
InterruptManager &InterruptManager::instance()
{
  return ARMV7InterruptManager::instance();
}

bool ARMV7InterruptManager::registerInterruptHandler(size_t interruptNumber, InterruptHandler *handler)
{
  /// \todo Needs locking
  if (UNLIKELY(interruptNumber >= 256))
    return false;
  if (UNLIKELY(handler != 0 && m_Handler[interruptNumber] != 0))
    return false;
  if (UNLIKELY(handler == 0 && m_Handler[interruptNumber] == 0))
    return false;

  m_Handler[interruptNumber] = handler;
  return true;
}

#ifdef DEBUGGER

  bool ARMV7InterruptManager::registerInterruptHandlerDebugger(size_t interruptNumber, InterruptHandler *handler)
  {
    /// \todo Needs locking
    if (UNLIKELY(interruptNumber >= 256))
      return false;
    if (UNLIKELY(handler != 0 && m_DbgHandler[interruptNumber] != 0))
      return false;
    if (UNLIKELY(handler == 0 && m_DbgHandler[interruptNumber] == 0))
      return false;

    m_DbgHandler[interruptNumber] = handler;
    return true;
  }
  size_t ARMV7InterruptManager::getBreakpointInterruptNumber()
  {
    return 3;
  }
  size_t ARMV7InterruptManager::getDebugInterruptNumber()
  {
    return 1;
  }

#endif

bool ARMV7InterruptManager::registerSyscallHandler(Service_t Service, SyscallHandler *handler)
{
  //TODO: Needs locking

  if (UNLIKELY(Service >= serviceEnd))
    return false;
  if (UNLIKELY(handler != 0 && m_SyscallHandler[Service] != 0))
    return false;
  if (UNLIKELY(handler == 0 && m_SyscallHandler[Service] == 0))
    return false;

  m_SyscallHandler[Service] = handler;
  return true;
}

uintptr_t ARMV7InterruptManager::syscall(Service_t service,
                                            uintptr_t function,
                                            uintptr_t p1, uintptr_t p2,
                                            uintptr_t p3, uintptr_t p4,
                                            uintptr_t p5)
{
    /// \todo Software interrupt
    return 0;
}

extern "C" void arm_swint_handler() __attribute__((interrupt("SWI")));
extern "C" void arm_instundef_handler() __attribute__((interrupt("UNDEF")));
extern "C" void arm_fiq_handler() __attribute__((interrupt("FIQ")));
extern "C" void arm_irq_handler() __attribute__((interrupt("IRQ")));
extern "C" void arm_reset_handler() __attribute__((interrupt("ABORT")));
extern "C" void arm_prefetch_abort_handler() __attribute__((interrupt("ABORT")));
extern "C" void arm_data_abort_handler() __attribute__((interrupt("ABORT")));
extern "C" void arm_addrexcept_handler() __attribute__((interrupt("ABORT")));

extern "C" void arm_swint_handler()
{
  NOTICE_NOLOCK("swi");
}

extern "C" void arm_instundef_handler()
{
  NOTICE_NOLOCK("undefined instruction");
  while( 1 );
}

extern "C" void arm_fiq_handler()
{
  NOTICE_NOLOCK("FIQ");
  while( 1 );
}

extern "C" void arm_irq_handler()
{
  NOTICE_NOLOCK("IRQ");
  while( 1 );
}

extern "C" void arm_reset_handler()
{
  NOTICE_NOLOCK("reset");
  while( 1 );
}

extern "C" void arm_prefetch_abort_handler()
{
  NOTICE_NOLOCK("prefetch abort");
  while( 1 );
}

extern "C" void arm_data_abort_handler()
{
  NOTICE_NOLOCK("data abort");
  while( 1 );
}

extern "C" void arm_addrexcept_handler()
{
  NOTICE_NOLOCK("address exception");
  while( 1 );
}

extern uint32_t __arm_vector_table;
extern uint32_t __end_arm_vector_table;
void ARMV7InterruptManager::initialiseProcessor()
{
    // Map in the ARM vector table to 0xFFFF0000
    if(!VirtualAddressSpace::getKernelAddressSpace().map(reinterpret_cast<physical_uintptr_t>(&__arm_vector_table),
                                                         reinterpret_cast<void*>(0xFFFF0000),
                                                         VirtualAddressSpace::Write | VirtualAddressSpace::KernelMode))
        return;

    // Switch to the high vector for the exception base
    uint32_t sctlr = 0;
    asm volatile("MRC p15,0,%0,c1,c0,0" : "=r" (sctlr));
    if(!(sctlr & 0x2000))
        sctlr |= 0x2000;
    asm volatile("MCR p15,0,%0,c1,c0,0" : : "r" (sctlr));
}

void ARMV7InterruptManager::interrupt(InterruptState &interruptState)
{
  /// \todo Needs locking
  size_t intNumber = interruptState.getInterruptNumber();

  #ifdef DEBUGGER
    // Call the kernel debugger's handler, if any
    if (m_Instance.m_DbgHandler[intNumber] != 0)
      m_Instance.m_DbgHandler[intNumber]->interrupt(intNumber, interruptState);
  #endif

  // Call the syscall handler, if it is the syscall interrupt
  if (intNumber == SYSCALL_INTERRUPT_NUMBER)
  {
    size_t serviceNumber = interruptState.getSyscallService();
    if (LIKELY(serviceNumber < serviceEnd && m_Instance.m_SyscallHandler[serviceNumber] != 0))
      m_Instance.m_SyscallHandler[serviceNumber]->syscall(interruptState);
  }
  // Call the normal interrupt handler, if any, otherwise
  else if (m_Instance.m_Handler[intNumber] != 0)
    m_Instance.m_Handler[intNumber]->interrupt(intNumber, interruptState);
  else
  {
    /// \todo: Check for debugger initialisation.
    static LargeStaticString e;
    e.clear();
    e.append ("Exception #");
    e.append (intNumber, 10);
    e.append (": \"");
    e.append (g_ExceptionNames[intNumber]);
    e.append ("\"");
#ifdef DEBUGGER
    Debugger::instance().start(interruptState, e);
#else
    panic(e);
#endif
  }
}

ARMV7InterruptManager::ARMV7InterruptManager()
{
  // Initialise the pointers to the interrupt handler
  for (size_t i = 0;i < 256;i++)
  {
    m_Handler[i] = 0;
    #ifdef DEBUGGER
      m_DbgHandler[i] = 0;
    #endif
  }

  // Initialise the pointers to the syscall handler
  for (size_t i = 0;i < serviceEnd;i++)
    m_SyscallHandler[i] = 0;
}
ARMV7InterruptManager::~ARMV7InterruptManager()
{

}