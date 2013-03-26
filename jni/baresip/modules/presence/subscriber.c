/**
 * @file subscriber.c Presence subscriber
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re.h>
#include <baresip.h>
#include "presence.h"


/*
 * Subscriber - we subscribe to the status information of N resources.
 *
 * For each entry in the address book marked with ;presence=p2p,
 * we send a SUBSCRIBE to that person, and expect to receive
 * a NOTIFY when her status changes.
 */


struct presence {
	struct le le;
	struct sipsub *sub;
	struct tmr tmr;
	enum presence_status status;
	unsigned failc;
	struct contact *contact;
};

static struct list presencel;


static void tmr_handler(void *arg);


static uint32_t wait_term(const struct sipevent_substate *substate)
{
	uint32_t wait;

	switch (substate->reason) {

	case SIPEVENT_DEACTIVATED:
	case SIPEVENT_TIMEOUT:
		wait = 5;
		break;

	case SIPEVENT_REJECTED:
	case SIPEVENT_NORESOURCE:
		wait = 3600;
		break;

	case SIPEVENT_PROBATION:
	case SIPEVENT_GIVEUP:
	default:
		wait = 300;
		if (pl_isset(&substate->retry_after))
			wait = max(wait, pl_u32(&substate->retry_after));
		break;
	}

	return wait;
}


static uint32_t wait_fail(unsigned failc)
{
	switch (failc) {

	case 1:  return 30;
	case 2:  return 300;
	case 3:  return 3600;
	default: return 86400;
	}
}


static void notify_handler(struct sip *sip, const struct sip_msg *msg,
			   void *arg)
{
	enum presence_status status = PRESENCE_CLOSED;
	struct presence *pres = arg;
	const struct sip_hdr *hdr;
	struct pl pl;

	pres->failc = 0;

	hdr = sip_msg_hdr(msg, SIP_HDR_CONTENT_TYPE);
	if (!hdr || 0 != pl_strcasecmp(&hdr->val, "application/pidf+xml")) {

		if (hdr)
			(void)re_printf("presence: unsupported"
					" content-type: '%r'\n",
					&hdr->val);

		sip_treplyf(NULL, NULL, sip, msg, false,
			    415, "Unsupported Media Type",
			    "Accept: application/pidf+xml\r\n"
			    "Content-Length: 0\r\n"
			    "\r\n");
		return;
	}

	if (!re_regex((const char *)mbuf_buf(msg->mb), mbuf_get_left(msg->mb),
		      "<status>[^<]*<basic>[^<]*</basic>[^<]*</status>",
		      NULL, &pl, NULL)) {

		if (!pl_strcasecmp(&pl, "open"))
			status = PRESENCE_OPEN;
	}

	if (!re_regex((const char *)mbuf_buf(msg->mb), mbuf_get_left(msg->mb),
		      "<rpid:away/>")) {

		status = PRESENCE_CLOSED;
	}
	else if (!re_regex((const char *)mbuf_buf(msg->mb),
			   mbuf_get_left(msg->mb),
			   "<rpid:busy/>")) {

		status = PRESENCE_BUSY;
	}
	else if (!re_regex((const char *)mbuf_buf(msg->mb),
			   mbuf_get_left(msg->mb),
			   "<rpid:on-the-phone/>")) {

		status = PRESENCE_BUSY;
	}

	(void)sip_treply(NULL, sip, msg, 200, "OK");

	contact_set_presence(pres->contact, status);
}


static void close_handler(int err, const struct sip_msg *msg,
			  const struct sipevent_substate *substate, void *arg)
{
	struct presence *pres = arg;
	uint32_t wait;

	pres->sub = mem_deref(pres->sub);

	(void)re_printf("presence: subscriber closed <%r>: ",
			&contact_addr(pres->contact)->auri);

	if (substate) {
		(void)re_printf("%s", sipevent_reason_name(substate->reason));
		wait = wait_term(substate);
	}
	else if (msg) {
		(void)re_printf("%u %r", msg->scode, &msg->reason);
		wait = wait_fail(++pres->failc);
	}
	else {
		(void)re_printf("%m", err);
		wait = wait_fail(++pres->failc);
	}

	(void)re_printf("; will retry in %u secs (failc=%u)\n",
			wait, pres->failc);

	tmr_start(&pres->tmr, wait * 1000, tmr_handler, pres);

	contact_set_presence(pres->contact, PRESENCE_UNKNOWN);
}


static void destructor(void *arg)
{
	struct presence *pres = arg;

	list_unlink(&pres->le);
	tmr_cancel(&pres->tmr);
	mem_deref(pres->contact);
	mem_deref(pres->sub);
}


static int auth_handler(char **username, char **password,
			const char *realm, void *arg)
{
	return ua_auth(arg, username, password, realm);
}


static int subscribe(struct presence *pres)
{
	const char *routev[1];
	struct ua *ua;
	char uri[256];
	int err;

	/* We use the first UA */
	ua = ua_find_aor(NULL);
	if (!ua) {
		(void)re_printf("presence: no UA found\n");
		return ENOENT;
	}

	pl_strcpy(&contact_addr(pres->contact)->auri, uri, sizeof(uri));

	routev[0] = ua_outbound(ua);

	err = sipevent_subscribe(&pres->sub, uag_sipevent_sock(), uri, NULL,
				 ua_aor(ua), "presence", NULL, 600,
				 ua_cuser(ua), routev, routev[0] ? 1 : 0,
				 auth_handler, ua_prm(ua), true, NULL,
				 notify_handler, close_handler, pres, NULL);
	if (err) {
		(void)re_fprintf(stderr,
				 "presence: sipevent_subscribe failed: %m\n",
				 err);
	}

	return err;
}


static void tmr_handler(void *arg)
{
	struct presence *pres = arg;

	if (subscribe(pres)) {
		tmr_start(&pres->tmr, wait_fail(++pres->failc) * 1000,
			  tmr_handler, pres);
	}
}


static int presence_alloc(struct contact *contact)
{
	struct presence *pres;

	pres = mem_zalloc(sizeof(*pres), destructor);
	if (!pres)
		return ENOMEM;

	pres->status  = PRESENCE_UNKNOWN;
	pres->contact = mem_ref(contact);

	tmr_init(&pres->tmr);
	tmr_start(&pres->tmr, 1000, tmr_handler, pres);

	list_append(&presencel, &pres->le, pres);

	return 0;
}


int subscriber_init(void)
{
	struct le *le;
	int err = 0;

	for (le = list_head(contact_list()); le; le = le->next) {

		struct contact *c = le->data;
		struct sip_addr *addr = contact_addr(c);
		struct pl val;

		if (0 == sip_param_decode(&addr->params, "presence", &val) &&
		    0 == pl_strcasecmp(&val, "p2p")) {

			err |= presence_alloc(le->data);
		}
	}

	(void)re_printf("Subscribing to %u contacts\n",
			list_count(&presencel));

	return err;
}


void subscriber_close(void)
{
	list_flush(&presencel);
}
