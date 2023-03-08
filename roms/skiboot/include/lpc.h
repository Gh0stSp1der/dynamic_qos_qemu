// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
/* Copyright 2013-2019 IBM Corp. */

#ifndef __LPC_H
#define __LPC_H

#include <opal.h>
#include <ccan/endian/endian.h>

/* Note about LPC interrupts
 *
 * LPC interrupts come in two categories:
 *
 *   - External device LPC interrupts
 *   - Error interrupts generated by the LPC controller
 *
 * The former is implemented differently depending on whether
 * you are using Murano/Venice or Naples.
 *
 * The former two chips don't have a pin to deserialize the LPC
 * SerIRQ protocol, so the only source of LPC device interrupts
 * is an external interrupt pin, which is usually connected to a
 * CPLD which deserializes SerIRQ.
 *
 * So in that case, we get external interrupts from the PSI which
 * are in effect the "OR" of all the active LPC interrupts.
 *
 * The error interrupt generated by the LPC controllers however
 * are internally routed normally to the PSI bridge and muxed with
 * the I2C interrupts.
 *
 * On Naples, there is a pin to deserialize SerIRQ, so the individual
 * LPC device interrupts (up to 19) are represented in the same status
 * and mask register as the LPC error interrupts. They are still all
 * then turned into a single XIVE interrupts in the PSI however, muxed
 * with the I2C.
 *
 * In order to more/less transparently handle this, we let individual
 * "drivers" register for specific LPC interrupts. On Naples, the handlers
 * will be called individually based on what has been demuxed by the
 * controller. On Venice/Murano, all the handlers will be called on
 * every external interrupt. The platform is responsible of calling
 * lpc_all_interrupts() from the platform external interrupt handler.
 */

/* Routines for accessing the LPC bus on Power8 */

extern void lpc_init(void);
extern void lpc_init_interrupts(void);
extern void lpc_finalize_interrupts(void);

/* Check for a default bus */
extern bool lpc_present(void);

/* Return of LPC is currently usable. This can be false if the caller
 * currently holds a lock that would make it unsafe, or the LPC bus
 * is known to be in some error condition (TBI).
 */
extern bool lpc_ok(void);

/* Handle the interrupt from the LPC controller */
extern void lpc_interrupt(uint32_t chip_id);

/* On P9, we have a different route for SerIRQ */
extern void lpc_serirq(uint32_t chip_id, uint32_t index);

/* Call all external handlers */
extern void lpc_all_interrupts(uint32_t chip_id);

/* Register/deregister handler */
struct lpc_client {
	/* Callback on LPC reset */
	void (*reset)(uint32_t chip_id);

	/* Callback on LPC interrupt */
	void (*interrupt)(uint32_t chip_id, uint32_t irq_msk);
	/* Bitmask of interrupts this client is interested in
	 * Note: beware of ordering, use LPC_IRQ() macro
	 */
	uint32_t interrupts;
#define LPC_IRQ(n)	(0x80000000 >> (n))
};

extern void lpc_register_client(uint32_t chip_id, const struct lpc_client *clt,
				uint32_t policy);

/* Return the policy for a given serirq */
extern unsigned int lpc_get_irq_policy(uint32_t chip_id, uint32_t psi_idx);

/* Default bus accessors that perform error logging */
extern int64_t lpc_write(enum OpalLPCAddressType addr_type, uint32_t addr,
			 uint32_t data, uint32_t sz);
extern int64_t lpc_read(enum OpalLPCAddressType addr_type, uint32_t addr,
			uint32_t *data, uint32_t sz);

/*
 * LPC bus accessors that return errors as required but do not log the failure.
 * Useful if the caller wants to test the presence of a device on the LPC bus.
 */
extern int64_t lpc_probe_write(enum OpalLPCAddressType addr_type, uint32_t addr,
			       uint32_t data, uint32_t sz);
extern int64_t lpc_probe_read(enum OpalLPCAddressType addr_type, uint32_t addr,
			      uint32_t *data, uint32_t sz);

/* Mark LPC bus as used by console */
extern void lpc_used_by_console(void);

/*
 * Simplified big endian FW accessors
 */
static inline int64_t lpc_fw_read32(uint32_t *val, uint32_t addr)
{
	return lpc_read(OPAL_LPC_FW, addr, val, 4);
}

static inline int64_t lpc_fw_write32(uint32_t val, uint32_t addr)
{
	return lpc_write(OPAL_LPC_FW, addr, val, 4);
}


/*
 * Simplified Little Endian IO space accessors
 *
 * Note: We do *NOT* handle unaligned accesses
 */

static inline void lpc_outb(uint8_t data, uint32_t addr)
{
	lpc_write(OPAL_LPC_IO, addr, data, 1);
}

static inline uint8_t lpc_inb(uint32_t addr)
{
	uint32_t d32;
	int64_t rc = lpc_read(OPAL_LPC_IO, addr, &d32, 1);
	return (rc == OPAL_SUCCESS) ? d32 : 0xff;
}

static inline void lpc_outw(uint16_t data, uint32_t addr)
{
	lpc_write(OPAL_LPC_IO, addr, cpu_to_le16(data), 2);
}

static inline uint16_t lpc_inw(uint32_t addr)
{
	uint32_t d32;
	int64_t rc = lpc_read(OPAL_LPC_IO, addr, &d32, 2);
	return (rc == OPAL_SUCCESS) ? le16_to_cpu(d32) : 0xffff;
}

static inline void lpc_outl(uint32_t data, uint32_t addr)
{
	lpc_write(OPAL_LPC_IO, addr, cpu_to_le32(data), 4);
}

static inline uint32_t lpc_inl(uint32_t addr)
{
	uint32_t d32;
	int64_t rc = lpc_read(OPAL_LPC_IO, addr, &d32, 4);
	return (rc == OPAL_SUCCESS) ? le32_to_cpu(d32) : 0xffffffff;
}

#endif /* __LPC_H */
