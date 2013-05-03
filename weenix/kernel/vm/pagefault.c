#include "types.h"
#include "globals.h"
#include "kernel.h"
#include "errno.h"

#include "util/debug.h"

#include "proc/proc.h"

#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/page.h"
#include "mm/mmobj.h"
#include "mm/pframe.h"
#include "mm/pagetable.h"

#include "vm/pagefault.h"
#include "vm/vmmap.h"

/*
 * This gets called by _pt_fault_handler in mm/pagetable.c The
 * calling function has already done a lot of error checking for
 * us. In particular it has checked that we are not page faulting
 * while in kernel mode. Make sure you understand why an
 * unexpected page fault in kernel mode is bad in Weenix. You
 * should probably read the _pt_fault_handler function to get a
 * sense of what it is doing.
 *
 * Before you can do anything you need to find the vmarea that
 * contains the address that was faulted on. Make sure to check
 * the permissions on the area to see if the process has
 * permission to do [cause]. If either of these checks does not
 * pass kill the offending process, setting its exit status to
 * EFAULT (normally we would send the SIGSEGV signal, however
 * Weenix does not support signals).
 *
 * Now it is time to find the correct page (don't forget
 * about shadow objects, especially copy-on-write magic!). Make
 * sure that if the user writes to the page it will be handled
 * correctly.
 *
 * Finally call pt_map to have the new mapping placed into the
 * appropriate page table.
 *
 * @param vaddr the address that was accessed to cause the fault
 *
 * @param cause this is the type of operation on the memory
 *              address which caused the fault, possible values
 *              can be found in pagefault.h
 */
 /*
 #define FAULT_PRESENT  0x01
#define FAULT_WRITE    0x02
#define FAULT_USER     0x04
#define FAULT_RESERVED 0x08
#define FAULT_EXEC     0x10
( ((cause & FAULT_WRITE) && (vma->vma_prot & PROT_NONE)) 
|| ((cause & FAULT_EXEC) && (vma->vma_prot & PROT_NONE)) 
|| ((cause & FAULT_WRITE) && (vma->vma_prot & PROT_WRITE)==0) 
|| ((cause & FAULT_EXEC) && (vma->vma_prot & PROT_EXEC)==0) )
#define PROT_NONE       0x0    
#define PROT_READ       0x1    
#define PROT_WRITE      0x2    
#define PROT_EXEC       0x4
 */
void
handle_pagefault(uintptr_t vaddr, uint32_t cause)
{
	dbg(DBG_VFS,"VM: Enter handle_pagefault()\n");
	vmarea_t *fault_vma=vmmap_lookup(curproc->p_vmmap, ADDR_TO_PN(vaddr));
	/*find vmarea*/
	if(fault_vma==NULL)
	{
		dbg(DBG_VFS,"VM: In handle_pagefault(), fault_vma==NULL\n");
		proc_kill(curproc, -EFAULT);
		dbg(DBG_VFS,"VM: Leave handle_pagefault(), fault_vma==NULL\n");
		return;
	}/*check permission*/
	else if(fault_vma->vma_prot&PROT_NONE)
	{
		dbg(DBG_VFS,"VM: In handle_pagefault()， check PROT_NONE\n");
		if(!(cause&FAULT_RESERVED))
		{
			proc_kill(curproc, -EFAULT);
			dbg(DBG_VFS,"VM: Leave handle_pagefault(), FAULT_RESERVED\n");
			return;
		}
	}
	else if(fault_vma->vma_prot&PROT_EXEC)
	{
		dbg(DBG_VFS,"VM: In handle_pagefault()， check PROT_EXEC\n");
		if(!(cause&FAULT_EXEC))
		{
			proc_kill(curproc, -EFAULT);
			dbg(DBG_VFS,"VM: Leave handle_pagefault(), FAULT_EXEC\n");
			return;
		}
	}
	else if(fault_vma->vma_prot & PROT_WRITE)
	{
		dbg(DBG_VFS,"VM: In handle_pagefault()， check PROT_WRITE\n");
		if(!(cause&FAULT_WRITE))
		{
			proc_kill(curproc, -EFAULT);
			dbg(DBG_VFS,"VM: Leave handle_pagefault(), FAULT_WRITE\n");
			return;
		}
	}
	dbg(DBG_VFS,"VM: In handle_pagefault()， to find correct page...\n");
	/*to find the correct page*/
	pframe_t *result_pframe;
	if(fault_vma->vma_flags==MAP_PRIVATE && fault_vma->vma_obj->mmo_shadowed!=NULL)
	{
		fault_vma->vma_obj->mmo_shadowed->mmo_ops->lookuppage(fault_vma->vma_obj->mmo_shadowed,ADDR_TO_PN(vaddr),cause&FAULT_WRITE,&result_pframe);
	}
	else if(fault_vma->vma_flags==MAP_SHARED)
	{
		fault_vma->vma_obj->mmo_ops->lookuppage(fault_vma->vma_obj,ADDR_TO_PN(vaddr),cause&FAULT_WRITE,&result_pframe);
	}
	uint32_t pdflags=FAULT_USER;
	uint32_t ptflags=FAULT_USER;
	if(cause & FAULT_PRESENT)
	{
		ptflags=ptflags|PT_PRESENT;
		pdflags=pdflags|PD_PRESENT;
	}
	if(cause & FAULT_WRITE)
	{
		ptflags=ptflags|PT_WRITE;
		pdflags=pdflags|PD_WRITE;
	}
	if(!PAGE_ALIGNED(vaddr))
		vaddr=ADDR_TO_PN(vaddr)*PAGE_SIZE;
	uintptr_t paddr = pt_virt_to_phys((uintptr_t)result_pframe->pf_addr);
	pt_map(curproc->p_pagedir,vaddr,paddr,pdflags,ptflags);
	dbg(DBG_VFS,"VM: Leave handle_pagefault()\n");
    /*NOT_YET_IMPLEMENTED("VM: handle_pagefault");*/
}
