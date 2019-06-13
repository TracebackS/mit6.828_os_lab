#include "ns.h"

extern union Nsipc nsipcbuf;
struct jif_pkt *pkt = (struct jif_pkt *)REQVA;

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";

	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server (using ipc_send with NSREQ_INPUT as value)
	//	do the above things in a loop
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.
	int r;
	int len;
	struct jif_pkt *p = pkt;
	for (int i = 0; i < 10; i++)
	{
		if ((r = sys_page_alloc(0, (void *)((uintptr_t)pkt + i * PGSIZE), PTE_P | PTE_U | PTE_W)) < 0)
		{
			panic("sys_page_alloc: %e\n", r);
		}
	}
	int c = 0;
	while (1)
	{
		while ((len = sys_net_recv((void *)((uintptr_t)p + sizeof(p->jp_len)), PGSIZE - sizeof(p->jp_len))) < 0)
		{
//			cprintf("len: %d\n", len);
			sys_yield();
		}
		p->jp_len = len;
		ipc_send(ns_envid, NSREQ_INPUT, p, PTE_P | PTE_U);
		c = (c + 1) % 10;
		p = (struct jif_pkt *)((uintptr_t)pkt + c * PGSIZE);
		sys_yield();
	}
}
