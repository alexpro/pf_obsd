/*	$OpenBSD: pfvar_priv.h,v 1.5 2018/09/11 07:53:38 sashan Exp $	*/

/*
 * Copyright (c) 2001 Daniel Hartmeier
 * Copyright (c) 2002 - 2013 Henning Brauer <henning@openbsd.org>
 * Copyright (c) 2016 Alexander Bluhm <bluhm@openbsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    - Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    - Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _NET_PFVAR_PRIV_H_
#define _NET_PFVAR_PRIV_H_

#ifdef _KERNEL

#include <sys/rwlock.h>

extern struct rwlock pf_lock;

/*For extra data in OpenBSD inpcb*/
struct obsd_pf_inpcb {
    struct inpcb *inp; //inpcb
    struct pf_state_key *inp_pf_sk;
};

/* from OpenBSD (mbuf.h) */

/* pkthdr_pf.flags */
#define	PF_TAG_GENERATED		0x01
#define	PF_TAG_SYNCOOKIE_RECREATED	0x02
#define	PF_TAG_TRANSLATE_LOCALHOST	0x04
#define	PF_TAG_DIVERTED			0x08
#define	PF_TAG_DIVERTED_PACKET		0x10
#define	PF_TAG_REROUTE			0x20
#define	PF_TAG_REFRAGMENTED		0x40	/* refragmented ipv6 packet */
#define	PF_TAG_PROCESSED		0x80	/* packet was checked by pf */

struct pkthdr_pf {
	struct pf_state_key *statekey;	/* pf stackside statekey */
	struct obsd_pf_inpcb	*oinp;		/* connected pcb for outgoing packet */
	u_int32_t	 qid;		/* queue id */
	u_int16_t	 tag;		/* tag id */
	u_int16_t	 delay;		/* delay packet by X ms */
	u_int8_t	 flags;
	u_int8_t	 routed;
	u_int8_t	 prio;
	u_int8_t	 pad[1];
};

/*FreeBSD workaround*/
struct mbuf_pf {
	struct mbuf *m;
	struct pkthdr_pf *ph_pf;
};

struct pf_pdesc {
	struct {
		int	 done;
		uid_t	 uid;
		gid_t	 gid;
		pid_t	 pid;
	}		 lookup;
	u_int64_t	 tot_len;	/* Make Mickey money */

	struct pf_addr	 nsaddr;	/* src address after NAT */
	struct pf_addr	 ndaddr;	/* dst address after NAT */

	struct pfi_kif	*kif;		/* incoming interface */
	struct mbuf	*m;		/* mbuf containing the packet */
	struct pkthdr_pf *ph_pf;	/* pkthdr_pf from OpenBSD */
	struct pf_addr	*src;		/* src address */
	struct pf_addr	*dst;		/* dst address */
	u_int16_t	*pcksum;	/* proto cksum */
	u_int16_t	*sport;
	u_int16_t	*dport;
	u_int16_t	 osport;
	u_int16_t	 odport;
	u_int16_t	 nsport;	/* src port after NAT */
	u_int16_t	 ndport;	/* dst port after NAT */

	u_int32_t	 off;		/* protocol header offset */
	u_int32_t	 hdrlen;	/* protocol header length */
	u_int32_t	 p_len;		/* length of protocol payload */
	u_int32_t	 extoff;	/* extentsion header offset */
	u_int32_t	 fragoff;	/* fragment header offset */
	u_int32_t	 jumbolen;	/* length from v6 jumbo header */
	u_int32_t	 badopts;	/* v4 options or v6 routing headers */

	u_int16_t	 rdomain;	/* original routing domain */
	u_int16_t	 virtual_proto;
#define PF_VPROTO_FRAGMENT	256
	sa_family_t	 af;
	sa_family_t	 naf;
	u_int8_t	 proto;
	u_int8_t	 tos;
	u_int8_t	 ttl;
	u_int8_t	 dir;		/* direction */
	u_int8_t	 sidx;		/* key index for source */
	u_int8_t	 didx;		/* key index for destination */
	u_int8_t	 destchg;	/* flag set when destination changed */
	u_int8_t	 pflog;		/* flags for packet logging */
	union {
		struct tcphdr			tcp;
		struct udphdr			udp;
		struct icmp			icmp;
#ifdef INET6
		struct icmp6_hdr		icmp6;
		struct mld_hdr			mld;
		struct nd_neighbor_solicit	nd_ns;
#endif /* INET6 */
	} hdr;
};

extern struct task	pf_purge_task;
extern struct callout	pf_purge_to;

struct pf_state		*pf_state_ref(struct pf_state *);
void			 pf_state_unref(struct pf_state *);

#ifdef WITH_PF_LOCK
extern struct rwlock	pf_lock;
extern struct rwlock	pf_state_lock;

#define PF_LOCK()		do {			\
		NET_ASSERT_LOCKED();			\
		rw_enter_write(&pf_lock);		\
	} while (0)

#define PF_UNLOCK()		do {			\
		PF_ASSERT_LOCKED();			\
		rw_exit_write(&pf_lock);		\
	} while (0)

#define PF_ASSERT_LOCKED()	do {			\
		if (rw_status(&pf_lock) != RW_WRITE)	\
			splassert_fail(RW_WRITE,	\
			    rw_status(&pf_lock),__func__);\
	} while (0)

#define PF_ASSERT_UNLOCKED()	do {			\
		if (rw_status(&pf_lock) == RW_WRITE)	\
			splassert_fail(0, rw_status(&pf_lock), __func__);\
	} while (0)

#define PF_STATE_ENTER_READ()	do {			\
		rw_enter_read(&pf_state_lock);		\
	} while (0)

#define PF_STATE_EXIT_READ()	do {			\
		rw_exit_read(&pf_state_lock);		\
	} while (0)

#define PF_STATE_ENTER_WRITE()	do {			\
		rw_enter_write(&pf_state_lock);		\
	} while (0)

#define PF_STATE_EXIT_WRITE()	do {			\
		PF_ASSERT_STATE_LOCKED();		\
		rw_exit_write(&pf_state_lock);		\
	} while (0)

#define PF_ASSERT_STATE_LOCKED()	do {		\
		if (rw_status(&pf_state_lock) != RW_WRITE)\
			splassert_fail(RW_WRITE,	\
			    rw_status(&pf_state_lock), __func__);\
	} while (0)

#else /* !WITH_PF_LOCK */

/*Temporary stubs below, TODO: make it work*/
#define KERNEL_LOCK()		(void)(0)
#define KERNEL_UNLOCK()		(void)(0)
#define NET_LOCK()		(void)(0)
#define NET_UNLOCK()		(void)(0)
/*End of stubs*/

#define PF_LOCK()		(void)(0)
#define PF_UNLOCK()		(void)(0)
#define PF_ASSERT_LOCKED()	(void)(0)
#define PF_ASSERT_UNLOCKED()	(void)(0)

#define PF_STATE_ENTER_READ()	(void)(0)
#define PF_STATE_EXIT_READ()	(void)(0)
#define PF_STATE_ENTER_WRITE()	(void)(0)
#define PF_STATE_EXIT_WRITE()	(void)(0)
#define PF_ASSERT_STATE_LOCKED()	(void)(0)

#endif /* WITH_PF_LOCK */

extern void			 pf_purge_timeout(void *);
extern void			 pf_purge(void *,int pending);
#endif /* _KERNEL */

#endif /* _NET_PFVAR_PRIV_H_ */
