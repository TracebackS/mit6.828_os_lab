#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/string.h>
#include <inc/error.h>

static struct E1000 *base;

struct tx_desc *tx_descs;
#define N_TXDESC (PGSIZE / sizeof(struct tx_desc))

struct packet
{
	char body[2048];
};

struct packet pbuf[N_TXDESC] = {{{0}}};

volatile void *e1000;

int
e1000_tx_init()
{
	// Allocate one page for descriptors

	// Initialize all descriptors

	// Set hardward registers
	// Look kern/e1000.h to find useful definations

	return 0;
}

struct rx_desc *rx_descs;
#define N_RXDESC (PGSIZE / sizeof(struct rx_desc))

int
e1000_rx_init()
{
	// Allocate one page for descriptors

	// Initialize all descriptors
	// You should allocate some pages as receive buffer

	// Set hardward registers
	// Look kern/e1000.h to find useful definations

	return 0;
}

int
pci_e1000_attach(struct pci_func *pcif)
{
	// Enable PCI function
	// Map MMIO region and save the address in 'base;

	e1000_tx_init();
	e1000_rx_init();
	pci_func_enable(pcif);
	for (int i = 0; i < N_TXDESC; i++)
	{
		memset(&tx_descs[i], 0, sizeof(tx_descs[i]));
		tx_descs[i].addr = PADDR(&pbuf[i]);
		tx_descs[i].status = E1000_TX_STATUS_DD;
		tx_descs[i].cmd = E1000_TX_CMD_RS | E1000_TX_CMD_EOP;
	}
	e1000 = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
	cprintf("status: 0x%lx\n", ((struct E1000 *)e1000)->STATUS);

	((struct E1000 *)e1000)->TDBAL = PADDR(tx_descs);
	((struct E1000 *)e1000)->TDBAH = 0;
	((struct E1000 *)e1000)->TDLEN = N_TXDESC * sizeof(struct tx_desc);
	((struct E1000 *)e1000)->TDH = 0;
	((struct E1000 *)e1000)->TDT = 0;
	((struct E1000 *)e1000)->TCTL = E1000_TCTL_EN | E1000_TCTL_PSP | E1000_TCTL_CT_ETHER | E1000_TCTL_COLD_FULL_DUPLEX;
	((struct E1000 *)e1000)->TIPG = 10 | (8 << 10) | (12 << 20);
	return 1;
}

int
e1000_tx(const void *buf, uint32_t len)
{
	// Send 'len' bytes in 'buf' to ethernet
	// Hint: buf is a kernel virtual address

	return 0;
}

int
e1000_rx(void *buf, uint32_t len)
{
	// Copy one received buffer to buf
	// You could return -E_AGAIN if there is no packet
	// Check whether the buf is large enough to hold
	// the packet
	// Do not forget to reset the decscriptor and
	// give it back to hardware by modifying RDT

	return 0;
}
