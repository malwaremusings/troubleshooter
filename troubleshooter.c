#include <windows.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <errno.h>
#include <ipexport.h>
#include <unistd.h>

IP_ADAPTER_ADDRESSES *getInterfaces() {
	IP_ADAPTER_ADDRESSES *ret = NULL;
	ULONG buffsize = 16384;
	DWORD err = 0;

	ret = malloc(buffsize);
	if (ret) {
		err = GetAdaptersAddresses(AF_INET,0,NULL,ret,&buffsize);
		if (err != ERROR_SUCCESS) {
			fprintf(stderr,"getInterfaces(): GetAdaptersAddresses() call returned %ld\n",err);
		}
	} else {
		fprintf(stderr,"getInterfaces(): malloc() call failed\n");
	}

	return ret;
}

IP_ADDR_STRING *getDNSServers() {
	FIXED_INFO *hostnetinfo = NULL;
	ULONG hostnetinfosize = 0;

	IP_ADDR_STRING *ret = NULL;
	ULONG err = 0;

	do {
		err = GetNetworkParams(hostnetinfo,&hostnetinfosize);
		if (err == ERROR_BUFFER_OVERFLOW) {
			hostnetinfo = malloc(hostnetinfosize);
		}
	} while (err == ERROR_BUFFER_OVERFLOW);

	if (err == ERROR_SUCCESS) {
		ret = &(hostnetinfo -> DnsServerList);
	} else {
		perror("getDNSServers(): GetNetworkParams() failed");
	}

	return ret;
}

MIB_IPNETTABLE *getARPTable() {
	MIB_IPNETTABLE *arptab = NULL;
	ULONG arptabsize = 0;

	MIB_IPNETTABLE *ret = NULL;
	ULONG err = 0;

	do {
		err = GetIpNetTable(arptab,&arptabsize,FALSE);
		if (err == ERROR_INSUFFICIENT_BUFFER) {
			arptab = malloc(arptabsize);
		}
	} while (err == ERROR_INSUFFICIENT_BUFFER);

	if (err == ERROR_SUCCESS) {
		ret = arptab;
	} else {
		perror("getARPTable(): GetIpNetTable() failed");
	}

	return ret;
}

MIB_IPFORWARDTABLE *getRoutingTable() {
	MIB_IPFORWARDTABLE *iproutetab = NULL;
	ULONG iproutetabsize = 0;

	MIB_IPFORWARDTABLE *ret = NULL;
	ULONG err = 0;

	do {
		err = GetIpForwardTable(iproutetab,&iproutetabsize,FALSE);
		if (err == ERROR_INSUFFICIENT_BUFFER) {
			iproutetab = malloc(iproutetabsize);
		}
	} while (err == ERROR_INSUFFICIENT_BUFFER);

	if (err == ERROR_SUCCESS) {
		ret = iproutetab;
	} else {
		perror("getRoutingTable(): getIpForwardTable() failed");
	}

	return ret;
}

MIB_IPNETROW *getArpEntry(struct in_addr a,MIB_IPNETTABLE *arptab) {
	/* returns a pointer to the arp table entry for address a */
	MIB_IPNETROW *host = NULL;

	if (arptab) {
		int i = 0;

		do {
			host = &((*arptab).table[i++]);
		} while ((host -> dwAddr != a.S_un.S_addr) && (i < (*arptab).dwNumEntries));
	} else {
		fprintf(stderr,"getArpEntry() called with NULL arp table\n");
	}

	return host;
}

MIB_IPFORWARDROW *getRouter(struct in_addr r,MIB_IPFORWARDTABLE *rttab) {
	/* returns a pointer to the routing table entry for router r */
	MIB_IPFORWARDROW *rtr = NULL;

	if (rttab) {
		int i = 0;

		do {
			rtr = &((*rttab).table[i++]);
		} while ((rtr -> dwForwardDest != r.S_un.S_addr) && (i < (*rttab).dwNumEntries));
	} else {
		fprintf(stderr,"getRouter() called with NULL routing table\n");
	}

	return rtr;
}

boolean checkRouter(struct in_addr r,MIB_IPNETTABLE *a) {
	/* Check router r has an arp entry in arp table a */

	int i = 0;
	boolean found = FALSE;
	MIB_IPNETROW arprow;

	if (a) {
		while ((i < a -> dwNumEntries) && (!found)) {
			arprow = (*a).table[i++];

			struct in_addr ipv4Addr;
			memcpy(&ipv4Addr,&(arprow.dwAddr),sizeof(ipv4Addr));

			found = (ipv4Addr.S_un.S_addr == r.S_un.S_addr);
		}
	} else {
		fprintf(stderr,"checkRouter() called with NULL ARP table\n");
	}

	return found;
}

boolean attemptConnect(struct in_addr *host) {
	int sockfd = -1;
	struct sockaddr_in sa_host;
	boolean ret = FALSE;

	WSADATA wsaData;
	int err = 0;

	err = WSAStartup(MAKEWORD(2,2),&wsaData);
	if (err == 0) {
		memset(&sa_host,0,sizeof(sa_host));
		sa_host.sin_family = AF_INET;
		sa_host.sin_port = htons(80);
		memcpy(&(sa_host.sin_addr),host,sizeof(sa_host.sin_addr));
	
		sockfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);		/* should getprotoent() to look up TCP */
		if (sockfd != -1) {
			if (connect(sockfd,(struct sockaddr *)&sa_host,sizeof(sa_host)) != -1) {
				ret = TRUE;
			} else {
				perror("attemptConnect(): connect() call failed");
			}
			close(sockfd);		/* for some reason this doesn't send a FIN. The connection is being RST instead! */
		} else {
			perror("attemptConnect(): socket() call failed");
		}
	} else {
		fprintf(stderr,"attemptConnect(): WSAStartup() failed");
	}

	return ret;
}

int WINAPI WinMain(HINSTANCE hThisInst,HINSTANCE hPrevInst,LPSTR lpszArgs,int nWinMode) {
	IP_ADDR_STRING *dnssvrlist;
	MIB_IPNETTABLE *arptable;
	MIB_IPFORWARDTABLE *iproutetab;


	dnssvrlist = getDNSServers();
	if (dnssvrlist) {
		printf("DNS SERVERS\n");
		printf("-----------\n");

		IP_ADDR_STRING *dnssvr = dnssvrlist;
		while (dnssvr) {
			printf("  %s\n",dnssvr -> IpAddress.String);
			dnssvr = dnssvr -> Next;
		}
		printf("\n\n");
	} else {
		fprintf(stderr,"Failed to get list of DNS servers\n");
	}

	arptable = getARPTable();
	if (arptable) {
		printf("ARP CACHE\n");
		printf("---------\n");

		int i = 0;
		for (i = 0;i < arptable -> dwNumEntries;i++) {
			MIB_IPNETROW arprow = (*arptable).table[i];

			printf("%ld: ",arprow.dwIndex);
			int j = 0;
			for (j = 0;j < arprow.dwPhysAddrLen;j++) {
				printf("%02x",arprow.bPhysAddr[j]);
			}
			printf(" ");

			struct in_addr ipv4Addr;
			memcpy(&ipv4Addr,&(arprow.dwAddr),sizeof(ipv4Addr));

			printf("%d.%d.%d.%d ",ipv4Addr.S_un.S_un_b.s_b1,ipv4Addr.S_un.S_un_b.s_b2,ipv4Addr.S_un.S_un_b.s_b3,ipv4Addr.S_un.S_un_b.s_b4);
			printf("%ld\n",arprow.dwType);
		}
		printf("\n\n");
	} else {
		fprintf(stderr,"Failed to get ARP table\n");
	}

	iproutetab = getRoutingTable();
	if (iproutetab) {
		printf("ROUTING TABLE\n");
		printf("-------------\n");

		int i = 0;
		for (i = 0;i < iproutetab -> dwNumEntries;i++) {
			MIB_IPFORWARDROW iproute = (*iproutetab).table[i];

			struct in_addr fwddst;
			memcpy(&fwddst,&(iproute.dwForwardDest),sizeof(fwddst));

			struct in_addr netmsk;
			memcpy(&netmsk,&(iproute.dwForwardMask),sizeof(netmsk));

			struct in_addr nxthop;
			memcpy(&nxthop,&(iproute.dwForwardNextHop),sizeof(nxthop));

			printf("%d.%d.%d.%d ",fwddst.S_un.S_un_b.s_b1,fwddst.S_un.S_un_b.s_b2,fwddst.S_un.S_un_b.s_b3,fwddst.S_un.S_un_b.s_b4);
			printf("%d.%d.%d.%d ",netmsk.S_un.S_un_b.s_b1,netmsk.S_un.S_un_b.s_b2,netmsk.S_un.S_un_b.s_b3,netmsk.S_un.S_un_b.s_b4);
			printf("%d.%d.%d.%d ",nxthop.S_un.S_un_b.s_b1,nxthop.S_un.S_un_b.s_b2,nxthop.S_un.S_un_b.s_b3,nxthop.S_un.S_un_b.s_b4);
			printf("\n");
		}
	} else {
		fprintf(stderr,"Failed to get IP routing table\n");
	}
	printf("\n\n");

	printf("INTERNET CONFIGURATION CHECKS\n");
	printf("-----------------------------\n");
	printf("Checking network interfaces:\n");

	IP_ADAPTER_ADDRESSES *iftab = getInterfaces();
	IP_ADAPTER_ADDRESSES *curr = iftab;
	while (curr != NULL) {
		WCHAR *ifFName = curr -> FriendlyName;
		DWORD ifFlags = curr -> Flags;
		DWORD ifType = curr -> IfType;
		IF_OPER_STATUS ifOperStatus = curr -> OperStatus;

		printf("    Interface\tType\tStatus\n");
		printf("    %s\t%ld\t%d\n",ifFName,ifType,ifOperStatus);
		curr = curr -> Next;
	}

	printf("INTERNET CONNECTIVITY CHECKS\n");
	printf("----------------------------\n");
	printf("Checking default gateway is set: ");
	
	struct in_addr rtr;
	rtr.S_un.S_addr = 0x00000000;
	MIB_IPFORWARDROW *defrtrrow = getRouter(rtr,iproutetab);
	struct in_addr defrtr;
	if (defrtrrow) {
		memcpy(&defrtr,&(defrtrrow -> dwForwardNextHop),sizeof(defrtr));
		printf("PASS (%d.%d.%d.%d)",defrtr.S_un.S_un_b.s_b1,defrtr.S_un.S_un_b.s_b2,defrtr.S_un.S_un_b.s_b3,defrtr.S_un.S_un_b.s_b4);
	} else {
		printf("FAIL");
	}
	printf("\n");

	printf("Checking default gateway is reachable: ");
	BYTE MacAddr[MAXLEN_PHYSADDR];
	ULONG MacAddrLen = sizeof(MacAddr);
	DWORD err = SendARP((IPAddr)defrtr.S_un.S_addr,INADDR_ANY,(ULONG *)&MacAddr,&MacAddrLen);
	if (err == NO_ERROR) {
	 	printf("PASS (");
	 	int i = 0;
	 	for (i = 0;i < MacAddrLen;i++) {
	 		printf("%02x",MacAddr[i]);
	 		if (i < MacAddrLen - 1) printf(":"); else printf(")");
		}
	} else {
		printf("FAIL");
	}
	printf("\n");

	printf("Checking the Internet is reachable: ");
	struct in_addr dst;
	dst.S_un.S_addr = htonl(0x4a7ded30);
	err = attemptConnect(&dst);		/* one of www.google.com's addresses */
	if (err) {
		printf("PASS (%d.%d.%d.%d)",dst.S_un.S_un_b.s_b1,dst.S_un.S_un_b.s_b2,dst.S_un.S_un_b.s_b3,dst.S_un.S_un_b.s_b4);
	} else {
		printf("FAIL");
	}

#if 0
   /* Message loop */
   while (GetMessage(&msg,NULL,0,0)) {
      if (!IsDialogMessage(hDialog,&msg)) {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
   }
#endif

   return 0;
}
