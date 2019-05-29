// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	
	if (!(err & FEC_WR))
	{
		cprintf("The va is 0x%x\n", (uint32_t)addr);
		cprintf("The Eip is 0x%x\n", utf->utf_eip);
		panic("pgfault: err wrong\n");
	}
	pte_t pte = uvpt[PGNUM(addr)];
	if (!(pte & PTE_COW))
	{
		panic("pgfault: perm wrong\n");
	}

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.

	if (sys_page_alloc(0, (void *)PFTEMP, PTE_U | PTE_W | PTE_P) < 0)
	{
		panic("pgfault: sys_page_alloc wrong\n");
	}
	addr = ROUNDDOWN(addr, PGSIZE);
	memcpy((void *)PFTEMP, addr, PGSIZE);
	if ((r = sys_page_map(0, (void *)PFTEMP, 0, addr, PTE_U | PTE_W | PTE_P)) < 0)
	{
		panic("The sys_page_map is wrong, errno: %d\n", r);
	}
	if ((r = sys_page_unmap(0, (void *)PFTEMP)) < 0)
	{
		panic("the sys_page_unmap is wrong, errno: %d\n", r);
	}
	return;
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
	pte_t pte = uvpt[PGNUM(pn * PGSIZE)];
	int perm = PTE_U | PTE_P;
	if (pte & PTE_W || pte & PTE_COW)
	{
		perm |= PTE_COW;
	}
	if ((r = sys_page_map(0, (void *)(pn * PGSIZE), envid, (void *)(pn * PGSIZE), perm)) < 0)
	{
		return r;
	}
	if ((pte & PTE_W || pte & PTE_COW)
			&& (r = sys_page_map(0, (void *)(pn * PGSIZE), 0, (void *)(pn * PGSIZE), perm)) < 0)
	{
		return r;
	}
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	extern void *_pgfault_upcall();
	set_pgfault_handler(pgfault);
	int child_eid = sys_exofork();
	if (child_eid < 0)
	{
		panic("fork: sys_exofork err: %d\n", child_eid);
	}
	if (!child_eid)
	{
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}
	int r = sys_env_set_pgfault_upcall(child_eid, _pgfault_upcall);
	if (r < 0)
	{
		panic("fork: sys_env_set_pgfault_upcall err: %d\n", r);
	}
	for (int i = 0; i * PGSIZE < USTACKTOP; i++)
	{
		if ((uvpd[PDX(i * PGSIZE)] & PTE_P) && (uvpt[PGNUM(i * PGSIZE)] & PTE_U) && (uvpt[PGNUM(i * PGSIZE)] & PTE_P))
		{
			r = duppage(child_eid, i);
			if (r < 0)
			{
				panic("fork: err %d\n", r);
			}
		}
	}
	r = sys_page_alloc(child_eid, (void *)(UXSTACKTOP - PGSIZE), PTE_U | PTE_W | PTE_P);
	if (r < 0)
	{
		panic("fork: err %d\n", r);
	}
	if (sys_env_set_status(child_eid, ENV_RUNNABLE) < 0)
	{
		panic("fork: sys_env_set_status err\n");
	}
	return child_eid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
