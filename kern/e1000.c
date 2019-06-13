#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/string.h>
#include <inc/error.h>

static struct E1000 *base;

struct tx_desc tx_descs[64];
#define N_TXDESC (PGSIZE / sizeof(struct tx_desc))

struct packet
{
	char body[2048];
};

struct packet pbuf[64] = {{{0}}};

volatile uint32_t *e1000;

int
e1000_tx_init()
{
	// Allocate one page for descriptors

	// Initialize all descriptors

	// Set hardward registers
	// Look kern/e1000.h to find useful definations
	for (int i = 0; i < 64; i++)
	{
		memset(&tx_descs[i], 0, sizeof(tx_descs[i]));
		tx_descs[i].addr = PADDR(&pbuf[i]);
		tx_descs[i].status = 1;
		tx_descs[i].cmd = 9;
	}

	return 0;
}

struct rx_desc rx_descs[128];
#define N_RXDESC (PGSIZE / sizeof(struct rx_desc))

struct packet prbuf[128] = {{{0}}};

int
e1000_rx_init()
{
	// Allocate one page for descriptors

	// Initialize all descriptors
	// You should allocate some pages as receive buffer

	// Set hardward registers
	// Look kern/e1000.h to find useful definations
	for (int i = 0; i < 128; i++)
	{
		memset(&rx_descs[i], 0, sizeof(rx_descs[i]));
		rx_descs[i].addr = PADDR(&prbuf[i]);
		rx_descs[i].status = 0;
	}

	return 0;
}

int
pci_e1000_attach(struct pci_func *pcif)
{
	// Enable PCI function
	// Map MMIO region and save the address in 'base;

	pci_func_enable(pcif);
	e1000_tx_init();
	e1000_rx_init();
	e1000 = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
	e1000[0xe00] = PADDR(tx_descs);
	e1000[0xe01] = 0;
	e1000[0xe02] = 64 * sizeof(struct tx_desc);
	e1000[0xe04] = 0;
	e1000[0xe06] = 0;
	e1000[0x100] = 0x2 | 0x8 | (0xff0 & (0x10 << 4)) | (0x3ff000 & (0x40 << 12));
	e1000[0x104] = 10 | (8 << 10) | (12 << 20);
	e1000[0x1500] = QEMU_MAC_LOW;
	e1000[0x1501] = QEMU_MAC_HIGH | 0x80000000;
	memset((void *)&e1000[0x1480], 0, 512);
	e1000[0x32] = 0;
	e1000[0x34] = 0;
	e1000[0xa00] = PADDR(rx_descs);
	e1000[0xa01] = 0;
	e1000[0xa02] = 128 * sizeof(struct rx_desc);
	e1000[0xa04] = 0;
	e1000[0xa06] = 128 - 1;
	e1000[0x40] = 2 | 0 | 0x4000000 | 0x8000;
	cprintf("status: 0x%lx\n", ((struct E1000 *)e1000)->STATUS);

	return 0;
}

int
e1000_tx(const void *buf, uint32_t len)
{
	// Send 'len' bytes in 'buf' to ethernet
	// Hint: buf is a kernel virtual address
	uint32_t tail = e1000[0xe06];
	struct tx_desc *next = &tx_descs[tail];
	if ((next->status & 1) != 1)
	{
		return -1;
	}
	if (len > 2048)
	{
		len = 2048;
	}
	memmove(&pbuf[tail], buf, len);
	next->length = (uint16_t)len;
	next->status = 0;
	e1000[0xe06] = (tail + 1) % 64;
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
	uint32_t tail = (e1000[0xa06] + 1) % 128;
	struct rx_desc *next = &rx_descs[tail];
	if ((next->status & 1) != 1)
	{
		return -1;
	}
	cprintf("next->status: %d\n", next->status);
	if (next->length < len)
	{
		len = next->length;
	}
	memmove(buf, &prbuf[tail], len);
	next->status = 0;
	e1000[0xa06] = tail;
//	cprintf("-_-_-_-_-_-_-========= len: %d\n", len);
	return len;
}
