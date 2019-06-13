#include "ns.h"

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet request (using ipc_recv)
	//	- send the packet to the device driver (using sys_net_send)
	//	do the above things in a loop
	int p;
	envid_t eid;
	while (1)
	{
		if (ipc_recv(&eid, &nsipcbuf, &p) == NSREQ_OUTPUT)
		{
			while (sys_net_send(nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len) < 0)
			{
				sys_yield();
			}
		}
	}
}
