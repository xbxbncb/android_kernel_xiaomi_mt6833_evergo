/*
 * IPV6 GSO/GRO offload support
 * Linux INET implementation
 *
 * Copyright (C) 2016 secunet Security Networks AG
 * Author: Steffen Klassert <steffen.klassert@secunet.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * ESP GRO support
 */

#include <linux/skbuff.h>
#include <linux/init.h>
#include <net/protocol.h>
#include <crypto/aead.h>
#include <crypto/authenc.h>
#include <linux/err.h>
#include <linux/module.h>
#include <net/ip.h>
#include <net/xfrm.h>
#include <net/esp.h>
#include <linux/scatterlist.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <net/ip6_route.h>
#include <net/ipv6.h>
#include <linux/icmpv6.h>

static __u16 esp6_nexthdr_esp_offset(struct ipv6hdr *ipv6_hdr, int nhlen)
{
	int off = sizeof(struct ipv6hdr);
	struct ipv6_opt_hdr *exthdr;

	if (likely(ipv6_hdr->nexthdr == NEXTHDR_ESP))
		return offsetof(struct ipv6hdr, nexthdr);

	while (off < nhlen) {
		exthdr = (void *)ipv6_hdr + off;
		if (exthdr->nexthdr == NEXTHDR_ESP)
			return off;

		off += ipv6_optlen(exthdr);
	}

	return 0;
}

static struct sk_buff *esp6_gro_receive(struct list_head *head,
					struct sk_buff *skb)
{
	int offset = skb_gro_offset(skb);
	struct xfrm_offload *xo;
	struct xfrm_state *x;
	__be32 seq;
	__be32 spi;
	int nhoff;
	int err;

	if (!pskb_pull(skb, offset))
		return NULL;

	if ((err = xfrm_parse_spi(skb, IPPROTO_ESP, &spi, &seq)) != 0)
		goto out;

	xo = xfrm_offload(skb);
	if (!xo || !(xo->flags & CRYPTO_DONE)) {
		err = secpath_set(skb);
		if (err)
			goto out;

		if (skb->sp->len == XFRM_MAX_DEPTH)
			goto out;

		x = xfrm_state_lookup(dev_net(skb->dev), skb->mark,
				      (xfrm_address_t *)&ipv6_hdr(skb)->daddr,
				      spi, IPPROTO_ESP, AF_INET6);
		if (!x)
			goto out;

		skb->sp->xvec[skb->sp->len++] = x;
		skb->sp->olen++;

		xo = xfrm_offload(skb);
		if (!xo) {
			xfrm_state_put(x);
			goto out;
		}
	}

	xo->flags |= XFRM_GRO;

	nhoff = esp6_nexthdr_esp_offset(ipv6_hdr(skb), offset);
	if (!nhoff)
		goto out;

	IP6CB(skb)->nhoff = nhoff;
	XFRM_TUNNEL_SKB_CB(skb)->tunnel.ip6 = NULL;
	XFRM_SPI_SKB_CB(skb)->family = AF_INET6;
	XFRM_SPI_SKB_CB(skb)->daddroff = offsetof(struct ipv6hdr, daddr);
	XFRM_SPI_SKB_CB(skb)->seq = seq;

	/* We don't need to handle errors from xfrm_input, it does all
	 * the error handling and frees the resources on error. */
	xfrm_input(skb, IPPROTO_ESP, spi, -2);

	return ERR_PTR(-EINPROGRESS);
out:
	skb_push(skb, offset);
	NAPI_GRO_CB(skb)->same_flow = 0;
	NAPI_GRO_CB(skb)->flush = 1;

	return NULL;
}

static void esp6_gso_encap(struct xfrm_state *x, struct sk_buff *skb)
{
	struct ip_esp_hdr *esph;
	struct ipv6hdr *iph = ipv6_hdr(skb);
	struct xfrm_offload *xo = xfrm_offload(skb);
	u8 proto = iph->nexthdr;

	skb_push(skb, -skb_network_offset(skb));

	if (x->outer_mode->encap == XFRM_MODE_TRANSPORT) {
		__be16 frag;

		ipv6_skip_exthdr(skb, sizeof(struct ipv6hdr), &proto, &frag);
	}

	esph = ip_esp_hdr(skb);
	*skb_mac_header(skb) = IPPROTO_ESP;

	esph->spi = x->id.spi;
	esph->seq_no = htonl(XFRM_SKB_CB(skb)->seq.output.low);

	xo->proto = proto;
}

static struct sk_buff *esp6_gso_segment(struct sk_buff *skb,
				        netdev_features_t features)
{
	struct xfrm_state *x;
	struct ip_esp_hdr *esph;
	struct crypto_aead *aead;
	netdev_features_t esp_features = features;
	struct xfrm_offload *xo = xfrm_offload(skb);

	if (!xo)
		return ERR_PTR(-EINVAL);

	if (!(skb_shinfo(skb)->gso_type & SKB_GSO_ESP))
		return ERR_PTR(-EINVAL);

	x = skb->sp->xvec[skb->sp->len - 1];
	aead = x->data;
	esph = ip_esp_hdr(skb);

	if (esph->spi != x->id.spi)
		return ERR_PTR(-EINVAL);

	if (!pskb_may_pull(skb, sizeof(*esph) + crypto_aead_ivsize(aead)))
		return ERR_PTR(-EINVAL);

	__skb_pull(skb, sizeof(*esph) + crypto_aead_ivsize(aead));

	skb->encap_hdr_csum = 1;

	if (!(features & NETIF_F_HW_ESP) || !x->xso.offload_handle ||
	    (x->xso.dev != skb->dev))
		esp_features = features & ~(NETIF_F_SG | NETIF_F_CSUM_MASK);

	xo->flags |= XFRM_GSO_SEGMENT;
	return x->outer_mode->gso_segment(x, skb, esp_features);
}

static int esp6_input_tail(struct xfrm_state *x, struct sk_buff *skb)
{
	struct crypto_aead *aead = x->data;
	struct xfrm_offload *xo = xfrm_offload(skb);

	if (!pskb_may_pull(skb, sizeof(struct ip_esp_hdr) + crypto_aead_ivsize(aead)))
		return -EINVAL;

	if (!(xo->flags & CRYPTO_DONE))
		skb->ip_summed = CHECKSUM_NONE;

	return esp6_input_done2(skb, 0);
}

static int esp6_xmit(struct xfrm_state *x, struct sk_buff *skb,  netdev_features_t features)
{
	int len;
	int err;
	int alen;
	int blksize;
	struct xfrm_offload *xo;
	struct ip_esp_hdr *esph;
	struct crypto_aead *aead;
	struct esp_info esp;
	bool hw_offload = true;
	__u32 seq;

	esp.inplace = true;

	xo = xfrm_offload(skb);

	if (!xo)
		return -EINVAL;

	if (!(features & NETIF_F_HW_ESP) || !x->xso.offload_handle ||
	    (x->xso.dev != skb->dev)) {
		xo->flags |= CRYPTO_FALLBACK;
		hw_offload = false;
	}

	esp.proto = xo->proto;

	/* skb is pure payload to encrypt */

	aead = x->data;
	alen = crypto_aead_authsize(aead);

	esp.tfclen = 0;
	/* XXX: Add support for tfc padding here. */

	blksize = ALIGN(crypto_aead_blocksize(aead), 4);
	esp.clen = ALIGN(skb->len + 2 + esp.tfclen, blksize);
	esp.plen = esp.clen - skb->len - esp.tfclen;
	esp.tailen = esp.tfclen + esp.plen + alen;

	if (!hw_offload || (hw_offload && !skb_is_gso(skb))) {
		esp.nfrags = esp6_output_head(x, skb, &esp);
		if (esp.nfrags < 0)
			return esp.nfrags;
	}

	seq = xo->seq.low;

	esph = ip_esp_hdr(skb);
	esph->spi = x->id.spi;

	skb_push(skb, -skb_network_offset(skb));

	if (xo->flags & XFRM_GSO_SEGMENT) {
		esph->seq_no = htonl(seq);
		if (!skb_is_gso(skb))
			xo->seq.low++;
		else
			xo->seq.low += skb_shinfo(skb)->gso_segs;
	}

	esp.seqno = cpu_to_be64(xo->seq.low + ((u64)xo->seq.hi << 32));
	len = skb->len - sizeof(struct ipv6hdr);

	if (len > IPV6_MAXPLEN)
		len = 0;

	ipv6_hdr(skb)->payload_len = htons(len);

	if (hw_offload)
		return 0;

	err = esp6_output_tail(x, skb, &esp);
	if (err)
		return err;

	secpath_reset(skb);

	if (skb_needs_linearize(skb, skb->dev->features) &&
	    __skb_linearize(skb))
		return -ENOMEM;
	return 0;
}

static const struct net_offload esp6_offload = {
	.callbacks = {
		.gro_receive = esp6_gro_receive,
		.gso_segment = esp6_gso_segment,
	},
};

static const struct xfrm_type_offload esp6_type_offload = {
	.description	= "ESP6 OFFLOAD",
	.owner		= THIS_MODULE,
	.proto	     	= IPPROTO_ESP,
	.input_tail	= esp6_input_tail,
	.xmit		= esp6_xmit,
	.encap		= esp6_gso_encap,
};

static int __init esp6_offload_init(void)
{
	if (xfrm_register_type_offload(&esp6_type_offload, AF_INET6) < 0) {
		pr_info("%s: can't add xfrm type offload\n", __func__);
		return -EAGAIN;
	}

	return inet6_add_offload(&esp6_offload, IPPROTO_ESP);
}

static void __exit esp6_offload_exit(void)
{
	if (xfrm_unregister_type_offload(&esp6_type_offload, AF_INET6) < 0)
		pr_info("%s: can't remove xfrm type offload\n", __func__);

	inet6_del_offload(&esp6_offload, IPPROTO_ESP);
}

module_init(esp6_offload_init);
module_exit(esp6_offload_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Steffen Klassert <steffen.klassert@secunet.com>");
MODULE_ALIAS_XFRM_OFFLOAD_TYPE(AF_INET6, XFRM_PROTO_ESP);
