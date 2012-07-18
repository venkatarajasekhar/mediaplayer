/*
Jason -- nmblookup start
*/

#include "includes.h"

extern bool AllowDebugChange;

static bool give_flags = false;
static bool use_bcast = true;
static bool got_bcast = false;
static struct sockaddr_storage bcast_addr;
static bool recursion_desired = false;
static bool translate_addresses = false;
static int ServerFD= -1;
static bool RootPort = false;
static bool find_status = false;

/****************************************************************************
 Open the socket communication.
**************************************************************************/

static bool open_sockets(void)
{
	struct sockaddr_storage ss;
	const char *sock_addr = lp_socket_address();

	if (!interpret_string_addr(&ss, sock_addr,
				AI_NUMERICHOST|AI_PASSIVE)) {
		DEBUG(0,("open_sockets: unable to get socket address "
					"from string %s", sock_addr));
		return false;
	}
	ServerFD = open_socket_in( SOCK_DGRAM,
				(RootPort ? 137 : 0),
				(RootPort ?   0 : 3),
				&ss, true );

	if (ServerFD == -1) {
		return false;
	}

	set_socket_options( ServerFD, "SO_BROADCAST" );

	DEBUG(3, ("Socket opened.\n"));
	return true;
}

/****************************************************************************
turn a node status flags field into a string
****************************************************************************/
static char *node_status_flags(unsigned char flags)
{
	static fstring ret;
	fstrcpy(ret,"");

	fstrcat(ret, (flags & 0x80) ? "<GROUP> " : "        ");
	if ((flags & 0x60) == 0x00) fstrcat(ret,"B ");
	if ((flags & 0x60) == 0x20) fstrcat(ret,"P ");
	if ((flags & 0x60) == 0x40) fstrcat(ret,"M ");
	if ((flags & 0x60) == 0x60) fstrcat(ret,"H ");
	if (flags & 0x10) fstrcat(ret,"<DEREGISTERING> ");
	if (flags & 0x08) fstrcat(ret,"<CONFLICT> ");
	if (flags & 0x04) fstrcat(ret,"<ACTIVE> ");
	if (flags & 0x02) fstrcat(ret,"<PERMANENT> ");

	return ret;
}

/****************************************************************************
 Turn the NMB Query flags into a string.
****************************************************************************/

static char *query_flags(int flags)
{
	static fstring ret1;
	fstrcpy(ret1, "");

	if (flags & NM_FLAGS_RS) fstrcat(ret1, "Response ");
	if (flags & NM_FLAGS_AA) fstrcat(ret1, "Authoritative ");
	if (flags & NM_FLAGS_TC) fstrcat(ret1, "Truncated ");
	if (flags & NM_FLAGS_RD) fstrcat(ret1, "Recursion_Desired ");
	if (flags & NM_FLAGS_RA) fstrcat(ret1, "Recursion_Available ");
	if (flags & NM_FLAGS_B)  fstrcat(ret1, "Broadcast ");

	return ret1;
}

/****************************************************************************
 Do a node status query.
****************************************************************************/

static void do_node_status(int fd,
		const char *name,
		int type,
		struct sockaddr_storage *pss)
{
	struct nmb_name nname;
	int count, i, j;
	NODE_STATUS_STRUCT *status;
	struct node_status_extra extra;
	fstring cleanname;
	char addr[INET6_ADDRSTRLEN];

	print_sockaddr(addr, sizeof(addr), pss);
	d_printf("Looking up status of %s\n",addr);
	make_nmb_name(&nname, name, type);
	status = node_status_query(fd, &nname, pss, &count, &extra);
	if (status) {
		for (i=0;i<count;i++) {
			pull_ascii_fstring(cleanname, status[i].name);
			for (j=0;cleanname[j];j++) {
				if (!isprint((int)cleanname[j])) {
					cleanname[j] = '.';
				}
			}
			d_printf("\t%-15s <%02x> - %s\n",
			       cleanname,status[i].type,
			       node_status_flags(status[i].flags));
		}
		d_printf("\n\tMAC Address = %02X-%02X-%02X-%02X-%02X-%02X\n",
				extra.mac_addr[0], extra.mac_addr[1],
				extra.mac_addr[2], extra.mac_addr[3],
				extra.mac_addr[4], extra.mac_addr[5]);
		d_printf("\n");
		SAFE_FREE(status);
	} else {
		d_printf("No reply from %s\n\n",addr);
	}
}


/****************************************************************************
 Send out one query.
****************************************************************************/

static bool query_one(const char *lookup, unsigned int lookup_type)
{
	int j, count, flags = 0;
	struct sockaddr_storage *ip_list=NULL;

	if (got_bcast) {
		char addr[INET6_ADDRSTRLEN];
		print_sockaddr(addr, sizeof(addr), &bcast_addr);
		d_printf("querying %s on %s\n", lookup, addr);
		ip_list = name_query(ServerFD,lookup,lookup_type,use_bcast,
				     use_bcast?true:recursion_desired,
				     &bcast_addr, &count, &flags, NULL);
	} else {
		const struct in_addr *bcast;
		for (j=iface_count() - 1;
		     !ip_list && j >= 0;
		     j--) {
			char addr[INET6_ADDRSTRLEN];
			struct sockaddr_storage bcast_ss;

			bcast = iface_n_bcast_v4(j);
			if (!bcast) {
				continue;
			}
			in_addr_to_sockaddr_storage(&bcast_ss, *bcast);
			print_sockaddr(addr, sizeof(addr), &bcast_ss);
			d_printf("querying %s on %s\n",
			       lookup, addr);
			ip_list = name_query(ServerFD,lookup,lookup_type,
					     use_bcast,
					     use_bcast?True:recursion_desired,
					     &bcast_ss,&count, &flags, NULL);
		}
	}

	if (!ip_list) {
		return false;
	}

	if (give_flags) {
		d_printf("Flags: %s\n", query_flags(flags));
	}

	for (j=0;j<count;j++) {
		char addr[INET6_ADDRSTRLEN];
		if (translate_addresses) {
			char h_name[MAX_DNS_NAME_LENGTH];
			h_name[0] = '\0';
			if (sys_getnameinfo((const struct sockaddr *)&ip_list[j],
					sizeof(struct sockaddr_storage),
					h_name, sizeof(h_name),
					NULL, 0,
					NI_NAMEREQD)) {
				continue;
			}
			d_printf("%s, ", h_name);
		}
		print_sockaddr(addr, sizeof(addr), &ip_list[j]);
		d_printf("%s %s<%02x>\n", addr,lookup, lookup_type);
		/* We can only do find_status if the ip address returned
		   was valid - ie. name_query returned true.
		 */
		if (find_status) {
			do_node_status(ServerFD, lookup,
					lookup_type, &ip_list[j]);
		}
	}

	free(ip_list);

	return (ip_list != NULL);
}


/****************************************************************************
  main program
****************************************************************************/
int nmblookup_main(int argc,char *argv[])
{
	int opt;
	unsigned int lookup_type = 0x0;
	fstring lookup;
	static bool find_master=False;
	static bool lookup_by_ip = False;
	poptContext pc;
	TALLOC_CTX *frame = talloc_stackframe();

	struct poptOption long_options[] = {
		POPT_AUTOHELP
		{ "broadcast", 'B', POPT_ARG_STRING, NULL, 'B', "Specify address to use for broadcasts", "BROADCAST-ADDRESS" },
		{ "flags", 'f', POPT_ARG_NONE, NULL, 'f', "List the NMB flags returned" },
		{ "unicast", 'U', POPT_ARG_STRING, NULL, 'U', "Specify address to use for unicast" },
		{ "master-browser", 'M', POPT_ARG_NONE, NULL, 'M', "Search for a master browser" },
		{ "recursion", 'R', POPT_ARG_NONE, NULL, 'R', "Set recursion desired in package" },
		{ "status", 'S', POPT_ARG_NONE, NULL, 'S', "Lookup node status as well" },
		{ "translate", 'T', POPT_ARG_NONE, NULL, 'T', "Translate IP addresses into names" },
		{ "root-port", 'r', POPT_ARG_NONE, NULL, 'r', "Use root port 137 (Win95 only replies to this)" },
		{ "lookup-by-ip", 'A', POPT_ARG_NONE, NULL, 'A', "Do a node status on <name> as an IP Address" },
		POPT_COMMON_SAMBA
		POPT_COMMON_CONNECTION
		{ 0, 0, 0, 0 }
	};

	*lookup = 0;

	load_case_tables();

	setup_logging(argv[0],True);

	pc = poptGetContext("nmblookup", argc, (const char **)argv,
			long_options, POPT_CONTEXT_KEEP_FIRST);

	poptSetOtherOptionHelp(pc, "<NODE> ...");

	while ((opt = poptGetNextOpt(pc)) != -1) {
		switch (opt) {
		case 'f':
			give_flags = true;
			break;
		case 'M':
			find_master = true;
			break;
		case 'R':
			recursion_desired = true;
			break;
		case 'S':
			find_status = true;
			break;
		case 'r':
			RootPort = true;
			break;
		case 'A':
			lookup_by_ip = true;
			break;
		case 'B':
			if (interpret_string_addr(&bcast_addr,
					poptGetOptArg(pc),
					NI_NUMERICHOST)) {
				got_bcast = True;
				use_bcast = True;
			}
			break;
		case 'U':
			if (interpret_string_addr(&bcast_addr,
					poptGetOptArg(pc),
					0)) {
				got_bcast = True;
				use_bcast = False;
			}
			break;
		case 'T':
			translate_addresses = !translate_addresses;
			break;
		}
	}

	poptGetArg(pc); /* Remove argv[0] */

	if(!poptPeekArg(pc)) {
		poptPrintUsage(pc, stderr, 0);
		exit(1);
	}

	if (!lp_load(get_dyn_CONFIGFILE(),True,False,False,True)) {
		fprintf(stderr, "Can't load %s - run testparm to debug it\n",
				get_dyn_CONFIGFILE());
	}

	load_interfaces();
	if (!open_sockets()) {
		return(1);
	}

	while(poptPeekArg(pc)) {
		char *p;
		struct in_addr ip;

		fstrcpy(lookup,poptGetArg(pc));

		if(lookup_by_ip) {
			struct sockaddr_storage ss;
			ip = interpret_addr2(lookup);
			in_addr_to_sockaddr_storage(&ss, ip);
			fstrcpy(lookup,"*");
			do_node_status(ServerFD, lookup, lookup_type, &ss);
			continue;
		}

		if (find_master) {
			if (*lookup == '-') {
				fstrcpy(lookup,"\01\02__MSBROWSE__\02");
				lookup_type = 1;
			} else {
				lookup_type = 0x1d;
			}
		}

		p = strchr_m(lookup,'#');
		if (p) {
			*p = '\0';
			sscanf(++p,"%x",&lookup_type);
		}

		if (!query_one(lookup, lookup_type)) {
			d_printf( "name_query failed to find name %s", lookup );
			if( 0 != lookup_type ) {
				d_printf( "#%02x", lookup_type );
			}
			d_printf( "\n" );
		}
	}

	poptFreeContext(pc);
	TALLOC_FREE(frame);
	return(0);
}
/*
Jason -- nmblookup end
*/
/*
Jason -- smbtree start
*/

enum tree_level {LEV_WORKGROUP, LEV_SERVER, LEV_SHARE};
static enum tree_level level = LEV_SHARE;

/* Holds a list of workgroups or servers */

struct smb_name_list {
        struct smb_name_list *prev, *next;
        char *name, *comment;
        uint32 server_type;
};

static struct smb_name_list *workgroups, *servers, *shares;

static void free_name_list(struct smb_name_list *list)
{
        while(list)
                DLIST_REMOVE(list, list);
}

static void add_name(const char *machine_name, uint32 server_type,
                     const char *comment, void *state)
{
        struct smb_name_list **name_list = (struct smb_name_list **)state;
        struct smb_name_list *new_name;

        new_name = SMB_MALLOC_P(struct smb_name_list);

        if (!new_name)
                return;

        ZERO_STRUCTP(new_name);

	new_name->name = SMB_STRDUP(machine_name);
	new_name->comment = SMB_STRDUP(comment);
        new_name->server_type = server_type;

	if (!new_name->name || !new_name->comment) {
		SAFE_FREE(new_name->name);
		SAFE_FREE(new_name->comment);
		SAFE_FREE(new_name);
		return;
	}

        DLIST_ADD(*name_list, new_name);
}

/****************************************************************************
  display tree of smb workgroups, servers and shares
****************************************************************************/
static bool get_workgroups(struct user_auth_info *user_info)
{
        struct cli_state *cli;
        struct sockaddr_storage server_ss;
	TALLOC_CTX *ctx = talloc_tos();
	char *master_workgroup = NULL;

        /* Try to connect to a #1d name of our current workgroup.  If that
           doesn't work broadcast for a master browser and then jump off
           that workgroup. */

	master_workgroup = talloc_strdup(ctx, lp_workgroup());
	if (!master_workgroup) {
		return false;
	}

        if (!use_bcast && !find_master_ip(lp_workgroup(), &server_ss)) {
                DEBUG(4, ("Unable to find master browser for workgroup %s, falling back to broadcast\n", 
			  master_workgroup));
				use_bcast = True;
		} else if(!use_bcast) {
			char addr[INET6_ADDRSTRLEN];
			print_sockaddr(addr, sizeof(addr), &server_ss);
			if (!(cli = get_ipc_connect(addr, &server_ss, user_info)))
				return False;
		}

		if (!(cli = get_ipc_connect_master_ip_bcast(talloc_tos(),
							user_info,
							&master_workgroup))) {
			DEBUG(4, ("Unable to find master browser by "
				  "broadcast\n"));
			return False;
        }

        if (!cli_NetServerEnum(cli, master_workgroup,
                               SV_TYPE_DOMAIN_ENUM, add_name, &workgroups))
                return False;

        return True;
}

/* Retrieve the list of servers for a given workgroup */

static bool get_servers(char *workgroup, struct user_auth_info *user_info)
{
        struct cli_state *cli;
        struct sockaddr_storage server_ss;
	char addr[INET6_ADDRSTRLEN];

        /* Open an IPC$ connection to the master browser for the workgroup */

        if (!find_master_ip(workgroup, &server_ss)) {
                DEBUG(4, ("Cannot find master browser for workgroup %s\n",
                          workgroup));
                return False;
        }

	print_sockaddr(addr, sizeof(addr), &server_ss);
        if (!(cli = get_ipc_connect(addr, &server_ss, user_info)))
                return False;

        if (!cli_NetServerEnum(cli, workgroup, SV_TYPE_ALL, add_name,
                               &servers))
                return False;

        return True;
}

static bool get_rpc_shares(struct cli_state *cli,
			   void (*fn)(const char *, uint32, const char *, void *),
			   void *state)
{
	NTSTATUS status;
	struct rpc_pipe_client *pipe_hnd;
	TALLOC_CTX *mem_ctx;
	WERROR werr;
	struct srvsvc_NetShareInfoCtr info_ctr;
	struct srvsvc_NetShareCtr1 ctr1;
	int i;
	uint32_t resume_handle = 0;
	uint32_t total_entries = 0;

	mem_ctx = talloc_new(NULL);
	if (mem_ctx == NULL) {
		DEBUG(0, ("talloc_new failed\n"));
		return False;
	}

	status = cli_rpc_pipe_open_noauth(cli, &ndr_table_srvsvc.syntax_id,
					  &pipe_hnd);

	if (!NT_STATUS_IS_OK(status)) {
		DEBUG(10, ("Could not connect to srvsvc pipe: %s\n",
			   nt_errstr(status)));
		TALLOC_FREE(mem_ctx);
		return False;
	}

	ZERO_STRUCT(info_ctr);
	ZERO_STRUCT(ctr1);

	info_ctr.level = 1;
	info_ctr.ctr.ctr1 = &ctr1;

	status = rpccli_srvsvc_NetShareEnumAll(pipe_hnd, mem_ctx,
					       pipe_hnd->desthost,
					       &info_ctr,
					       0xffffffff,
					       &total_entries,
					       &resume_handle,
					       &werr);

	if (!NT_STATUS_IS_OK(status) || !W_ERROR_IS_OK(werr)) {
		TALLOC_FREE(mem_ctx);
		TALLOC_FREE(pipe_hnd);
		return False;
	}

	for (i=0; i<total_entries; i++) {
		struct srvsvc_NetShareInfo1 info = info_ctr.ctr.ctr1->array[i];
		fn(info.name, info.type, info.comment, state);
	}

	TALLOC_FREE(mem_ctx);
	TALLOC_FREE(pipe_hnd);
	return True;
}


static bool get_shares(char *server_name, struct user_auth_info *user_info)
{
        struct cli_state *cli;

        if (!(cli = get_ipc_connect(server_name, NULL, user_info)))
        {
                set_cmdline_auth_info_username(user_info, "");
                set_cmdline_auth_info_password(user_info, "");
                if (!(cli = get_ipc_connect(server_name, NULL, user_info)))
                    return False;
        }

	if (get_rpc_shares(cli, add_name, &shares))
		return True;

        if (cli_RNetShareEnum(cli, add_name, &shares)>0)
            return True;
        
        return False;
/*
        if (!cli_RNetShareEnum(cli, add_name, &shares))
                return False;

        return True;
*/
}

static bool print_tree(struct user_auth_info *user_info)
{
        struct smb_name_list *wg, *sv, *sh;

        /* List workgroups */

        if (!get_workgroups(user_info))
                return False;

        for (wg = workgroups; wg; wg = wg->next) {

                printf("%s\n", wg->name);

                /* List servers */

                free_name_list(servers);
                servers = NULL;

                if (level == LEV_WORKGROUP || 
                    !get_servers(wg->name, user_info))
                        continue;

                for (sv = servers; sv; sv = sv->next) {

                        printf("\t\\\\%-15s\t\t%s\n", 
			       sv->name, sv->comment);

                        /* List shares */

                        free_name_list(shares);
                        shares = NULL;

                        if (level == LEV_SERVER ||
                            !get_shares(sv->name, user_info))
                                continue;

                        for (sh = shares; sh; sh = sh->next) {
                                printf("\t\t\\\\%s\\%-15s\t%s\n", 
				       sv->name, sh->name, sh->comment);
                        }
                }
        }

        return True;
}

/****************************************************************************
  main program
****************************************************************************/
 int smbtree_main(int argc,char *argv[])
{
	TALLOC_CTX *frame = talloc_stackframe();
	struct user_auth_info *auth_info;
	struct poptOption long_options[] = {
		POPT_AUTOHELP
		{ "broadcast", 'b', POPT_ARG_VAL, &use_bcast, True, "Use broadcast instead of using the master browser" },
		{ "domains", 'D', POPT_ARG_VAL, &level, LEV_WORKGROUP, "List only domains (workgroups) of tree" },
		{ "servers", 'S', POPT_ARG_VAL, &level, LEV_SERVER, "List domains(workgroups) and servers of tree" },
		POPT_COMMON_SAMBA
		POPT_COMMON_CREDENTIALS
		POPT_TABLEEND
	};
	poptContext pc;

	/* Initialise samba stuff */
	load_case_tables();

	setlinebuf(stdout);

	dbf = x_stderr;

	setup_logging(argv[0],True);

	auth_info = user_auth_info_init(frame);
	if (auth_info == NULL) {
		exit(1);
	}
	popt_common_set_auth_info(auth_info);

	pc = poptGetContext("smbtree", argc, (const char **)argv, long_options,
						POPT_CONTEXT_KEEP_FIRST);
	while(poptGetNextOpt(pc) != -1);
	poptFreeContext(pc);

	lp_load(get_dyn_CONFIGFILE(),True,False,False,True);
	load_interfaces();

	/* Parse command line args */

	if (get_cmdline_auth_info_use_machine_account(auth_info) &&
	    !set_cmdline_auth_info_machine_account_creds(auth_info)) {
		TALLOC_FREE(frame);
		return 1;
	}

	set_cmdline_auth_info_getpass(auth_info);

	/* Now do our stuff */

        if (!print_tree(auth_info)) {
		TALLOC_FREE(frame);
                return 1;
	}

	TALLOC_FREE(frame);
	return 0;
}
/*
Jason -- smbtree end
*/
/*
Jason -- customer start
*/

#define JS_CONFIGURE_FILE "/tmp/netb/smb.conf"
int Samba_NetBiosName2Ip_needfree(char *lookup,struct sockaddr_storage **nip_list,int *n_ip_count)
{
    unsigned int lookup_type = 0x0;
    load_case_tables();
    if (!lp_load(JS_CONFIGURE_FILE,True,False,False,True)) {
        fprintf(stderr, "Can't load %s - run testparm to debug it\n",
        JS_CONFIGURE_FILE);
    }
    load_interfaces();
    if (!open_sockets()) {
        gfree_all();
        return(1);
    }
    {
        char *p;
        struct sockaddr_storage *ip_list=NULL;
        int j, count, flags = 0;
        const struct in_addr *bcast;
        
        p = strchr_m(lookup,'#');
        if (p) {
            *p = '\0';
            sscanf(++p,"%x",&lookup_type);
        }

        for (j=iface_count() - 1;
            !ip_list && j >= 0;
            j--) 
        {
            struct sockaddr_storage bcast_ss;

            bcast = iface_n_bcast_v4(j);
            if (!bcast) {
                continue;
            }
            in_addr_to_sockaddr_storage(&bcast_ss, *bcast);
/* for show log            
            {
                char addr[INET6_ADDRSTRLEN];
                print_sockaddr(addr, sizeof(addr), &bcast_ss);
                d_printf("querying %s on %s\n",lookup, addr);
            }
*/            
            ip_list = name_query(ServerFD,lookup,lookup_type,
                use_bcast,
                use_bcast?True:recursion_desired,
                &bcast_ss,&count, &flags, NULL);
        }
        if (!ip_list)
        {
            gfree_all();
            return -1;
        }
        *n_ip_count=count;
        *nip_list=ip_list;
        gfree_all();
    }
    return 0;
}
int Samba_NetBiosName2Ip(char *lookup,struct in_addr *ip)
{
    int ret;
    struct sockaddr_storage *n_ip=NULL;
    int count;
    if((ret=Samba_NetBiosName2Ip_needfree(lookup,&n_ip,&count))==0)
    {
        if(count>0)
        {
            int j;
            char addr[INET6_ADDRSTRLEN];
            for (j=0;j<count;j++) 
            {
                if(n_ip[j].ss_family == AF_INET) 
                {
                    print_sockaddr(addr, sizeof(addr), &n_ip[j]);
                    ip->s_addr=inet_addr(addr);
                    break;
                }
            }        
        }
        if(n_ip)
            free(n_ip);
    }
    return ret;
}
int Samba_QueryDomainList(long *Handle,int *num)
{
    TALLOC_CTX *frame = talloc_stackframe();
    struct user_auth_info *auth_info;
    int js_argc=1;
    char *js_argv[]={"smbtree"};
    struct poptOption long_options[] = {
        POPT_AUTOHELP
        { "broadcast", 'b', POPT_ARG_VAL, &use_bcast, True, "Use broadcast instead of using the master browser" },
        { "domains", 'D', POPT_ARG_VAL, &level, LEV_WORKGROUP, "List only domains (workgroups) of tree" },
        { "servers", 'S', POPT_ARG_VAL, &level, LEV_SERVER, "List domains(workgroups) and servers of tree" },
        POPT_COMMON_SAMBA
        POPT_COMMON_CREDENTIALS
        POPT_TABLEEND
    };
    poptContext pc;

    /* Initialise samba stuff */
    load_case_tables();

//    setlinebuf(stdout);

    dbf = x_stderr;

//    setup_logging(argv[0],True);

    auth_info = user_auth_info_init(frame);
    if (auth_info == NULL) {
        gfree_all();
        return -1;
    }
    popt_common_set_auth_info(auth_info);

    pc = poptGetContext("smbtree", js_argc, (const char **)js_argv, long_options,
    POPT_CONTEXT_KEEP_FIRST);
    
    while(poptGetNextOpt(pc) != -1);
    poptFreeContext(pc);

    lp_load(JS_CONFIGURE_FILE,True,False,False,True);
    load_interfaces();

    /* Parse command line args */

    if (get_cmdline_auth_info_use_machine_account(auth_info) &&
    !set_cmdline_auth_info_machine_account_creds(auth_info)) {
        gfree_all();
        TALLOC_FREE(frame);
        return 1;
    }

//    /* disable user input password */
//    set_cmdline_auth_info_getpass(auth_info);

    /* Now do our stuff */

    workgroups = NULL;
    /* List workgroups */

    if (!get_workgroups(auth_info))
    {
        gfree_all();
        TALLOC_FREE(frame);
        return -1;
    }

    {
        struct smb_name_list *wg;
        int lnum=0;

        for (wg = workgroups; wg; wg = wg->next) {
            lnum++;
        }
        *Handle=(long)(workgroups);
        *num=lnum;
    }
    gfree_all();
    TALLOC_FREE(frame);
    return 0;
}
int Samba_GetNameList(long Handle,int num,char **Name)
{
    int i;
    struct smb_name_list *p_nlTemp;
    p_nlTemp=(struct smb_name_list *)Handle;
    for(i=0;i<num;i++)
    {
        if(p_nlTemp==NULL)
            return -1;
        p_nlTemp=p_nlTemp->next;
    }
    *Name=p_nlTemp->name;
    return 0;
}
int Samba_GetDomainList(long Handle,int num,char **Domain)
{
    return Samba_GetNameList(Handle,num,Domain);
}
int Samba_QueryServerList(long *Handle,int *num,char *domain)
{
    TALLOC_CTX *frame = talloc_stackframe();
    struct user_auth_info *auth_info;
    int js_argc=1;
    char *js_argv[]={"smbtree"};
    struct poptOption long_options[] = {
        POPT_AUTOHELP
        { "broadcast", 'b', POPT_ARG_VAL, &use_bcast, True, "Use broadcast instead of using the master browser" },
        { "domains", 'D', POPT_ARG_VAL, &level, LEV_WORKGROUP, "List only domains (workgroups) of tree" },
        { "servers", 'S', POPT_ARG_VAL, &level, LEV_SERVER, "List domains(workgroups) and servers of tree" },
        POPT_COMMON_SAMBA
        POPT_COMMON_CREDENTIALS
        POPT_TABLEEND
    };
    poptContext pc;

    /* Initialise samba stuff */
    load_case_tables();

//    setlinebuf(stdout);

    dbf = x_stderr;

//    setup_logging(argv[0],True);

    auth_info = user_auth_info_init(frame);
    if (auth_info == NULL) {
        gfree_all();
        return -1;
    }
    popt_common_set_auth_info(auth_info);

    pc = poptGetContext("smbtree", js_argc, (const char **)js_argv, long_options,
    POPT_CONTEXT_KEEP_FIRST);
    
    while(poptGetNextOpt(pc) != -1);
    poptFreeContext(pc);

    lp_load(JS_CONFIGURE_FILE,True,False,False,True);
    load_interfaces();

    /* Parse command line args */

    if (get_cmdline_auth_info_use_machine_account(auth_info) &&
    !set_cmdline_auth_info_machine_account_creds(auth_info)) {
        gfree_all();
        TALLOC_FREE(frame);
        return 1;
    }

//    /* disable user input password */
//    set_cmdline_auth_info_getpass(auth_info);

    /* Now do our stuff */

    servers = NULL;
    /* List servers */

    if (!get_servers(domain,auth_info))
    {
        gfree_all();
        TALLOC_FREE(frame);
        return -1;
    }

    {
        struct smb_name_list *sv;
        int lnum=0;

        for (sv = servers; sv; sv = sv->next) {
            lnum++;
        }
        *Handle=(long)(servers);
        *num=lnum;
    }
    gfree_all();
    TALLOC_FREE(frame);
    return 0;
}
int Samba_GetServerList(long Handle,int num,char **Server)
{
    return Samba_GetNameList(Handle,num,Server);
}

int Samba_freeHandle(long Handle)
{
    struct smb_name_list *p_nlTemp;
    p_nlTemp=(struct smb_name_list *)Handle;
    free_name_list(p_nlTemp);
    p_nlTemp = NULL;
    return 0;
}

struct JSST_SHARELIST {
        char *name;
        uint32 type;
};
typedef struct JSST_SHARELIST JS_SHARELIST;

int Samba_QueryServerShareList(long *Handle,int *num,char *ip,char *p_username,char *p_password,char *p_domain)
{
    TALLOC_CTX *frame = talloc_stackframe();
    struct user_auth_info *auth_info;
    int js_argc=1;
    char *js_argv[]={"smbtree"};
    struct poptOption long_options[] = {
        POPT_AUTOHELP
        { "broadcast", 'b', POPT_ARG_VAL, &use_bcast, True, "Use broadcast instead of using the master browser" },
        { "domains", 'D', POPT_ARG_VAL, &level, LEV_WORKGROUP, "List only domains (workgroups) of tree" },
        { "servers", 'S', POPT_ARG_VAL, &level, LEV_SERVER, "List domains(workgroups) and servers of tree" },
        POPT_COMMON_SAMBA
        POPT_COMMON_CREDENTIALS
        POPT_TABLEEND
    };
    poptContext pc;

    /* Initialise samba stuff */
    load_case_tables();

//    setlinebuf(stdout);

    dbf = x_stderr;

//    setup_logging(argv[0],True);

    auth_info = user_auth_info_init(frame);
    if (auth_info == NULL) {
        gfree_all();
        return -1;
    }
    popt_common_set_auth_info(auth_info);

    pc = poptGetContext("smbtree", js_argc, (const char **)js_argv, long_options,
    POPT_CONTEXT_KEEP_FIRST);
    
    while(poptGetNextOpt(pc) != -1);
    poptFreeContext(pc);

    lp_load(JS_CONFIGURE_FILE,True,False,False,True);
    load_interfaces();

    /* Parse command line args */

    if (get_cmdline_auth_info_use_machine_account(auth_info) &&
    !set_cmdline_auth_info_machine_account_creds(auth_info)) {
        gfree_all();
        TALLOC_FREE(frame);
        return 1;
    }

//    /* disable user input password */
//    set_cmdline_auth_info_getpass(auth_info);
    if(p_username)
        set_cmdline_auth_info_username(auth_info, p_username);
    if(p_password)
        set_cmdline_auth_info_password(auth_info, p_password);
    if(p_domain)
        set_global_myworkgroup(p_domain);

    /* Now do our stuff */

    shares = NULL;
    /* List servers */

    if (!get_shares(ip,auth_info))
    {
        gfree_all();
        TALLOC_FREE(frame);
        return -1;
    }

    {
        struct smb_name_list *sh;
        int lnum=0;

        for (sh = shares; sh; sh = sh->next) {
            lnum++;
        }
        *Handle=(long)(shares);
        *num=lnum;
    }
    gfree_all();
    TALLOC_FREE(frame);
    return 0;
}
/*
Type:
#define STYPE_DISKTREE  0	
#define STYPE_PRINTQ    1	
#define STYPE_DEVICE    2	
#define STYPE_IPC       3	
*/
int Samba_GetServerShareList(long Handle,int num,char **Share,int *type)
{
    int i;
    struct smb_name_list *p_nlTemp;
    p_nlTemp=(struct smb_name_list *)Handle;
    for(i=0;i<num;i++)
    {
        if(p_nlTemp==NULL)
            return -1;
        p_nlTemp=p_nlTemp->next;
    }
    *Share=p_nlTemp->name;
    *type=p_nlTemp->server_type&0x07;
    return 0;
}
int Samba_freeServerShareHandle(long Handle,int Total)
{
    return Samba_freeHandle(Handle);
}
#ifdef EXECUTION_FILE
int main(int argc,char *argv[])
{
    int iCase;
    int i;
    for (i=1;i<argc;i++)
    {
        iCase=atoi(argv[i]);
        switch(iCase)
        {
        #if 0
        case 1:
            {
                nmblookup_main(argc,argv);
                smbtree_main(argc,argv);
            }
            break;
        #else
        case 1:
            printf("************************ test NetBiosName2Ip******************\n");
            {
                struct in_addr ip;
                if(!Samba_NetBiosName2Ip("AAAA",&ip))
                    d_printf("IP : %s\n",inet_ntoa(ip));
                else
                    d_printf("Can't find any IP : \n");
                
                if(!Samba_NetBiosName2Ip("WHQL_HD",&ip))
                    d_printf("IP : %s\n",inet_ntoa(ip));    
            }
            break;
        case 2:
            printf("************************ test NetBiosName2Ip iplist******************\n");
            {
                struct sockaddr_storage *n_ip=NULL;
                int count;
                if(Samba_NetBiosName2Ip_needfree("WHQL_HD",&n_ip,&count)==0)
                {
                    if(count>0)
                    {
                        int j;
                        char addr[INET6_ADDRSTRLEN];
                        for (j=0;j<count;j++) 
                        {
                            if(n_ip[j].ss_family == AF_INET) 
                            {
                                print_sockaddr(addr, sizeof(addr), &n_ip[j]);
                                d_printf("querying %d = %s\n",j, addr);
                            }
                        }        
                    }
                    if(n_ip)
                        free(n_ip);
                }
            }
            break;
        case 3:
            printf("************************ test QueryDomainList******************\n");
            {
                long Handle;
                int num=0;
                int i;
                if(Samba_QueryDomainList(&Handle,&num))
                {
                    printf("Get Fail\n");
                }
                else
                {
                    for (i=0;i<num;i++)
                    {
                        char *cTemp=NULL;
                        if(!Samba_GetDomainList(Handle,i,&cTemp))
                            printf("domain %d , %s \n",i,cTemp);
                        else
                            printf("domain fail to get %d \n",i);
                    }
                    Samba_freeHandle(Handle);
                }
            }    
            break;
        case 4:
            printf("************************ test QueryServerList******************\n");
            {
                long Handle;
                int num=0;
                int i;
                if(Samba_QueryServerList(&Handle,&num,"RTDOMAIN"))
                {
                    printf("Get Fail\n");
                }
                else
                {
                    for (i=0;i<num;i++)
                    {
                        char *cTemp=NULL;
                        if(!Samba_GetServerList(Handle,i,&cTemp))
                            printf("server %d , %s \n",i,cTemp);
                        else
                            printf("server fail to get %d \n",i);
                    }
                    Samba_freeHandle(Handle);
                }
            }    
            break;
        case 5:
            printf("************************ QueryServerShareList ******************\n");
            {
                long Handle;
                int num=0,i;
                char ip[]="SQA-PC";
                char p_username[]="aaa";
                char p_password[]="bbb";
                if(Samba_QueryServerShareList(&Handle,&num, ip, p_username, p_password,0))
                {
                    printf("Get Fail\n");
                }
                else
               {
                    printf("Windows 7 QueryServerShareList Num = %d\n",num);
                    for(i=0;i<num;i++)
                    {
                        char *Share;
                        int type;
                        Samba_GetServerShareList(Handle,i,&Share,&type);
                        printf("Share Name = %s , Type = %d \n",Share,type);
                    }
                    Samba_freeServerShareHandle(Handle,num);
                }
            }
            break;
        case 6:
            printf("************************ QueryServerShareList ******************\n");
            {
                long Handle;
                int num=0,i;
                char ip[]="172.21.98.181";
                char p_username[]="aaa";
                char p_password[]="bbb";
                if(Samba_QueryServerShareList(&Handle,&num, ip, p_username, p_password,0))
                {
                    printf("Get Fail\n");
                }
                else
               {
                    printf("Windows 7 QueryServerShareList Num = %d\n",num);
                    for(i=0;i<num;i++)
                    {
                        char *Share;
                        int type;
                        Samba_GetServerShareList(Handle,i,&Share,&type);
                        printf("Share Name = %s , Type = %d \n",Share,type);
                    }
                    Samba_freeServerShareHandle(Handle,num);
                }
            }
            break;
        case 7:
            printf("************************ QueryServerShareList ******************\n");
            {
                long Handle;
                int num=0,i;
                char ip[]="172.21.98.202";
                char p_username[]="jason";
                char p_password[]="XXX";
                if(Samba_QueryServerShareList(&Handle,&num, ip, p_username, p_password,0))
                {
                    printf("Get Fail\n");
                }
                else
               {
                    printf("Windows 7 QueryServerShareList Num = %d\n",num);
                    for(i=0;i<num;i++)
                    {
                        char *Share;
                        int type;
                        Samba_GetServerShareList(Handle,i,&Share,&type);
                        printf("Share Name = %s , Type = %d \n",Share,type);
                    }
                    Samba_freeServerShareHandle(Handle,num);
                }
            }
            break;
        case 8:
            printf("************************ QueryServerShareList ******************\n");
            {
                long Handle;
                int num=0,i;
                char ip[]="172.21.98.202";
                char p_username[]="jasonlee";
                char p_password[]="XXXX";
                char p_domain[]="RTDOMAIN";
                if(Samba_QueryServerShareList(&Handle,&num, ip, p_username, p_password,p_domain))
                {
                    printf("Get Fail\n");
                }
                else
                {
                    printf("QueryServerShareList Num = %d\n",num);
                    for(i=0;i<num;i++)
                    {
                        char *Share;
                        int type;
                        Samba_GetServerShareList(Handle,i,&Share,&type);
                        printf("Share Name = %s , Type = %d \n",Share,type);
                    }
                    Samba_freeServerShareHandle(Handle,num);
                }
            }
            break;
        #endif    
        }
    }
    return 0;
}
#endif
/*
Jason -- customer end
*/

