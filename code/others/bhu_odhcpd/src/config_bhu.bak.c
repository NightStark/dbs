#define _GNU_SOURCE 
#include <fcntl.h>
#include <resolv.h>
#include <signal.h>
#include <arpa/inet.h>
 #include <fcntl.h>   
 #include <unistd.h>


//#include <uci.h>
//#include <uci_blob.h>

#include "odhcpd.h"

static struct blob_buf b;
static int reload_pipe[2];
struct list_head leases = LIST_HEAD_INIT(leases);
struct list_head interfaces = LIST_HEAD_INIT(interfaces);
struct config config = {false, NULL, NULL};

enum {
	IFACE_ATTR_INTERFACE,
	IFACE_ATTR_IFNAME,
	IFACE_ATTR_NETWORKID,
	IFACE_ATTR_DYNAMICDHCP,
	IFACE_ATTR_IGNORE,
	IFACE_ATTR_LEASETIME,
	IFACE_ATTR_LIMIT,
	IFACE_ATTR_START,
	IFACE_ATTR_MASTER,
	IFACE_ATTR_UPSTREAM,
	IFACE_ATTR_RA,
	IFACE_ATTR_DHCPV4,
	IFACE_ATTR_DHCPV6,
	IFACE_ATTR_NDP,
	IFACE_ATTR_ROUTER,
	IFACE_ATTR_DNS,
	IFACE_ATTR_DOMAIN,
	IFACE_ATTR_FILTER_CLASS,
	IFACE_ATTR_DHCPV6_RAW,
	IFACE_ATTR_RA_DEFAULT,
	IFACE_ATTR_RA_MANAGEMENT,
	IFACE_ATTR_RA_OFFLINK,
	IFACE_ATTR_RA_PREFERENCE,
	IFACE_ATTR_RA_ADVROUTER,
	IFACE_ATTR_RA_MAXINTERVAL,
	IFACE_ATTR_PD_MANAGER,
	IFACE_ATTR_PD_CER,
	IFACE_ATTR_NDPROXY_ROUTING,
	IFACE_ATTR_NDPROXY_SLAVE,
	IFACE_ATTR_MAX
};

static const struct blobmsg_policy iface_attrs[IFACE_ATTR_MAX] = {
	[IFACE_ATTR_INTERFACE] = { .name = "interface", .type = BLOBMSG_TYPE_STRING },
	[IFACE_ATTR_IFNAME] = { .name = "ifname", .type = BLOBMSG_TYPE_STRING },
	[IFACE_ATTR_NETWORKID] = { .name = "networkid", .type = BLOBMSG_TYPE_STRING },
	[IFACE_ATTR_DYNAMICDHCP] = { .name = "dynamicdhcp", .type = BLOBMSG_TYPE_BOOL },
	[IFACE_ATTR_IGNORE] = { .name = "ignore", .type = BLOBMSG_TYPE_BOOL },
	[IFACE_ATTR_LEASETIME] = { .name = "leasetime", .type = BLOBMSG_TYPE_STRING },
	[IFACE_ATTR_START] = { .name = "start", .type = BLOBMSG_TYPE_INT32 },
	[IFACE_ATTR_LIMIT] = { .name = "limit", .type = BLOBMSG_TYPE_INT32 },
	[IFACE_ATTR_MASTER] = { .name = "master", .type = BLOBMSG_TYPE_BOOL },
	[IFACE_ATTR_UPSTREAM] = { .name = "upstream", .type = BLOBMSG_TYPE_ARRAY },
	[IFACE_ATTR_RA] = { .name = "ra", .type = BLOBMSG_TYPE_STRING },
	[IFACE_ATTR_DHCPV4] = { .name = "dhcpv4", .type = BLOBMSG_TYPE_STRING },
	[IFACE_ATTR_DHCPV6] = { .name = "dhcpv6", .type = BLOBMSG_TYPE_STRING },
	[IFACE_ATTR_NDP] = { .name = "ndp", .type = BLOBMSG_TYPE_STRING },
	[IFACE_ATTR_ROUTER] = { .name = "router", .type = BLOBMSG_TYPE_ARRAY },
	[IFACE_ATTR_DNS] = { .name = "dns", .type = BLOBMSG_TYPE_ARRAY },
	[IFACE_ATTR_DOMAIN] = { .name = "domain", .type = BLOBMSG_TYPE_ARRAY },
	[IFACE_ATTR_FILTER_CLASS] = { .name = "filter_class", .type = BLOBMSG_TYPE_STRING },
	[IFACE_ATTR_DHCPV6_RAW] = { .name = "dhcpv6_raw", .type = BLOBMSG_TYPE_STRING },
	[IFACE_ATTR_PD_MANAGER] = { .name = "pd_manager", .type = BLOBMSG_TYPE_STRING },
	[IFACE_ATTR_PD_CER] = { .name = "pd_cer", .type = BLOBMSG_TYPE_STRING },
	[IFACE_ATTR_RA_DEFAULT] = { .name = "ra_default", .type = BLOBMSG_TYPE_INT32 },
	[IFACE_ATTR_RA_MANAGEMENT] = { .name = "ra_management", .type = BLOBMSG_TYPE_INT32 },
	[IFACE_ATTR_RA_OFFLINK] = { .name = "ra_offlink", .type = BLOBMSG_TYPE_BOOL },
	[IFACE_ATTR_RA_PREFERENCE] = { .name = "ra_preference", .type = BLOBMSG_TYPE_STRING },
	[IFACE_ATTR_RA_ADVROUTER] = { .name = "ra_advrouter", .type = BLOBMSG_TYPE_BOOL },
	[IFACE_ATTR_RA_MAXINTERVAL] = { .name = "ra_maxinterval", .type = BLOBMSG_TYPE_INT32 },
	[IFACE_ATTR_NDPROXY_ROUTING] = { .name = "ndproxy_routing", .type = BLOBMSG_TYPE_BOOL },
	[IFACE_ATTR_NDPROXY_SLAVE] = { .name = "ndproxy_slave", .type = BLOBMSG_TYPE_BOOL },
};

/*
static const struct uci_blob_param_info iface_attr_info[IFACE_ATTR_MAX] = {
	[IFACE_ATTR_UPSTREAM] = { .type = BLOBMSG_TYPE_STRING },
	[IFACE_ATTR_DNS] = { .type = BLOBMSG_TYPE_STRING },
	[IFACE_ATTR_DOMAIN] = { .type = BLOBMSG_TYPE_STRING },
};
*/

/*
const struct uci_blob_param_list interface_attr_list = {
	.n_params = IFACE_ATTR_MAX,
	.params = iface_attrs,
	.info = iface_attr_info,
};
*/


enum {
	LEASE_ATTR_IP,
	LEASE_ATTR_MAC,
	LEASE_ATTR_DUID,
	LEASE_ATTR_HOSTID,
	LEASE_ATTR_LEASETIME,
	LEASE_ATTR_NAME,
	LEASE_ATTR_MAX
};


static const struct blobmsg_policy lease_attrs[LEASE_ATTR_MAX] = {
	[LEASE_ATTR_IP] = { .name = "ip", .type = BLOBMSG_TYPE_STRING },
	[LEASE_ATTR_MAC] = { .name = "mac", .type = BLOBMSG_TYPE_STRING },
	[LEASE_ATTR_DUID] = { .name = "duid", .type = BLOBMSG_TYPE_STRING },
	[LEASE_ATTR_HOSTID] = { .name = "hostid", .type = BLOBMSG_TYPE_STRING },
	[LEASE_ATTR_LEASETIME] = { .name = "leasetime", .type = BLOBMSG_TYPE_STRING },
	[LEASE_ATTR_NAME] = { .name = "name", .type = BLOBMSG_TYPE_STRING },
};


/*
const struct uci_blob_param_list lease_attr_list = {
	.n_params = LEASE_ATTR_MAX,
	.params = lease_attrs,
};
*/


enum {
	ODHCPD_ATTR_MAINDHCP,
	ODHCPD_ATTR_LEASEFILE,
	ODHCPD_ATTR_LEASETRIGGER,
	ODHCPD_ATTR_MAX
};


static const struct blobmsg_policy odhcpd_attrs[LEASE_ATTR_MAX] = {
	[ODHCPD_ATTR_MAINDHCP] = { .name = "maindhcp", .type = BLOBMSG_TYPE_BOOL },
	[ODHCPD_ATTR_LEASEFILE] = { .name = "leasefile", .type = BLOBMSG_TYPE_STRING },
	[ODHCPD_ATTR_LEASETRIGGER] = { .name = "leasetrigger", .type = BLOBMSG_TYPE_STRING },
};


/*
const struct uci_blob_param_list odhcpd_attr_list = {
	.n_params = ODHCPD_ATTR_MAX,
	.params = odhcpd_attrs,
};
*/


static struct interface* get_interface(const char *name)
{
	struct interface *c;
	list_for_each_entry(c, &interfaces, head)
		if (!strcmp(c->name, name))
			return c;
	return NULL;
}


static void clean_interface(struct interface *iface)
{
	free(iface->dns);
	free(iface->search);
	free(iface->upstream);
	free(iface->dhcpv4_router);
	free(iface->dhcpv4_dns);
	free(iface->dhcpv6_raw);
	free(iface->filter_class);
	memset(&iface->ra, 0, sizeof(*iface) - offsetof(struct interface, ra));
}


static void close_interface(struct interface *iface)
{
	if (iface->head.next)
		list_del(&iface->head);

	setup_router_interface(iface, false);
	setup_dhcpv6_interface(iface, false);
	setup_ndp_interface(iface, false);
	setup_dhcpv4_interface(iface, false);

	clean_interface(iface);
	free(iface);
}


static int parse_mode(const char *mode)
{
	if (!strcmp(mode, "disabled")) {
		return RELAYD_DISABLED;
	} else if (!strcmp(mode, "server")) {
		return RELAYD_SERVER;
	} else if (!strcmp(mode, "relay")) {
		return RELAYD_RELAY;
	} else if (!strcmp(mode, "hybrid")) {
		return RELAYD_HYBRID;
	} else {
		return -1;
	}
}

void odhcpd_reload(void)
{
    /*
	struct uci_context *uci = uci_alloc_context();
    */
	while (!list_empty(&leases)) {
		struct lease *l = list_first_entry(&leases, struct lease, head);
		list_del(&l->head);
		free(l->duid);
		free(l);
	}

	struct interface *master = NULL, *i, *n;

    /*
	if (!uci)
		return;
        */

	list_for_each_entry(i, &interfaces, head)
		clean_interface(i);

    /*
	struct uci_package *dhcp = NULL;
	if (!uci_load(uci, "dhcp", &dhcp)) {
		struct uci_element *e;
		uci_foreach_element(&dhcp->sections, e) {
			struct uci_section *s = uci_to_section(e);
			if (!strcmp(s->type, "host"))
				set_lease(s);
			else if (!strcmp(s->type, "odhcpd"))
				set_config(s);
		}

		uci_foreach_element(&dhcp->sections, e) {
			struct uci_section *s = uci_to_section(e);
			if (!strcmp(s->type, "dhcp"))
				set_interface(s);
		}
	}
    */


#ifdef WITH_UBUS
	ubus_apply_network();
#endif

	bool any_dhcpv6_slave = false, any_ra_slave = false, any_ndp_slave = false;

	// Test for
	list_for_each_entry(i, &interfaces, head) {
		if (i->master)
			continue;

		if (i->dhcpv6 == RELAYD_HYBRID || i->dhcpv6 == RELAYD_RELAY)
			any_dhcpv6_slave = true;

		if (i->ra == RELAYD_HYBRID || i->ra == RELAYD_RELAY)
			any_ra_slave = true;

		if (i->ndp == RELAYD_HYBRID || i->ndp == RELAYD_RELAY)
			any_ndp_slave = true;
	}

	// Evaluate hybrid mode for master
	list_for_each_entry(i, &interfaces, head) {
		if (!i->master)
			continue;

		enum odhcpd_mode hybrid_mode = RELAYD_DISABLED;
#ifdef WITH_UBUS
		if (!ubus_has_prefix(i->name, i->ifname))
			hybrid_mode = RELAYD_RELAY;
#endif

		if (i->dhcpv6 == RELAYD_HYBRID)
			i->dhcpv6 = hybrid_mode;

		if (i->dhcpv6 == RELAYD_RELAY && !any_dhcpv6_slave)
			i->dhcpv6 = RELAYD_DISABLED;

		if (i->ra == RELAYD_HYBRID)
			i->ra = hybrid_mode;

		if (i->ra == RELAYD_RELAY && !any_ra_slave)
			i->ra = RELAYD_DISABLED;

		if (i->ndp == RELAYD_HYBRID)
			i->ndp = hybrid_mode;

		if (i->ndp == RELAYD_RELAY && !any_ndp_slave)
			i->ndp = RELAYD_DISABLED;

		if (i->dhcpv6 == RELAYD_RELAY || i->ra == RELAYD_RELAY || i->ndp == RELAYD_RELAY)
			master = i;
	}


	list_for_each_entry_safe(i, n, &interfaces, head) {
		if (i->inuse) {
			// Resolve hybrid mode
			if (i->dhcpv6 == RELAYD_HYBRID)
				i->dhcpv6 = (master && master->dhcpv6 == RELAYD_RELAY) ?
						RELAYD_RELAY : RELAYD_SERVER;

			if (i->ra == RELAYD_HYBRID)
				i->ra = (master && master->ra == RELAYD_RELAY) ?
						RELAYD_RELAY : RELAYD_SERVER;

			if (i->ndp == RELAYD_HYBRID)
				i->ndp = (master && master->ndp == RELAYD_RELAY) ?
						RELAYD_RELAY : RELAYD_DISABLED;

			setup_router_interface(i, true);
			setup_dhcpv6_interface(i, true);
			setup_ndp_interface(i, true);
			setup_dhcpv4_interface(i, true);
		} else {
			close_interface(i);
		}
	}

	//uci_unload(uci, dhcp);
	//uci_free_context(uci);
}

static void reload_cb(struct uloop_fd *u, _unused unsigned int events)
{
	char b[512];
	if (read(u->fd, b, sizeof(b)) < 0) {}
	odhcpd_reload();
}

static void handle_signal(int signal)
{
	char b[1] = {0};

	if (signal == SIGHUP) {
		if (write(reload_pipe[1], b, sizeof(b)) < 0) {}
	} else
		uloop_end();
}
static struct uloop_fd reload_fd = { .cb = reload_cb };

void odhcpd_run(void)
{
    /*
	if (pipe2(reload_pipe, O_NONBLOCK | O_CLOEXEC) < 0) {}
	reload_fd.fd = reload_pipe[0];
	uloop_fd_add(&reload_fd, ULOOP_READ);
    */

	signal(SIGTERM, handle_signal);
	signal(SIGINT, handle_signal);
	signal(SIGHUP, handle_signal);

#ifdef WITH_UBUS
	while (init_ubus())
		sleep(1);
#endif

	odhcpd_reload();
	uloop_run();

	while (!list_empty(&interfaces))
		close_interface(list_first_entry(&interfaces, struct interface, head));
}
