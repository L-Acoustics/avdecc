/*
* Copyright (C) 2016-2026, L-Acoustics and its contributors

* This file is part of LA_avdecc.

* LA_avdecc is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* LA_avdecc is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.

* You should have received a copy of the GNU Lesser General Public License
* along with LA_avdecc.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "pcapInterface.hpp"

namespace la
{
namespace avdecc
{
namespace protocol
{
struct PcapInterface::pImpl
{
};

PcapInterface::PcapInterface() = default;
PcapInterface::~PcapInterface() = default;

bool PcapInterface::is_available() const
{
	return true;
}

const char* PcapInterface::lib_version() const
{
	return pcap_lib_version();
}

pcap_t* PcapInterface::create(const char* source, char* errbuf) const
{
	return pcap_create(source, errbuf);
}

int PcapInterface::set_snaplen(pcap_t* p, int snaplen) const
{
	return pcap_set_snaplen(p, snaplen);
}

int PcapInterface::set_promisc(pcap_t* p, int promisc) const
{
	return pcap_set_promisc(p, promisc);
}

int PcapInterface::set_timeout(pcap_t* p, int to_ms) const
{
	return pcap_set_timeout(p, to_ms);
}

#ifdef _WIN32
int PcapInterface::setmintocopy(pcap_t* p, int size) const
{
	return pcap_setmintocopy(p, size);
}
#else // !_WIN32
int PcapInterface::set_immediate_mode(pcap_t* p, int immediate) const
{
	return pcap_set_immediate_mode(p, immediate);
}
#endif // _WIN32

int PcapInterface::activate(pcap_t* p) const
{
	return pcap_activate(p);
}

char* PcapInterface::geterr(pcap_t* p) const
{
	return pcap_geterr(p);
}

int PcapInterface::fileno(pcap_t* p) const
{
	return pcap_fileno(p);
}

void PcapInterface::close(pcap_t* p) const
{
	pcap_close(p);
}

int PcapInterface::compile(pcap_t* p, struct bpf_program* fp, const char* str, int optimize, bpf_u_int32 netmask) const
{
	return pcap_compile(p, fp, str, optimize, netmask);
}

int PcapInterface::setfilter(pcap_t* p, struct bpf_program* fp) const
{
	return pcap_setfilter(p, fp);
}

void PcapInterface::freecode(bpf_program* fp) const
{
	pcap_freecode(fp);
}

int PcapInterface::next_ex(pcap_t* p, struct pcap_pkthdr** pkt_header, const u_char** pkt_data) const
{
	return pcap_next_ex(p, pkt_header, pkt_data);
}

int PcapInterface::loop(pcap_t* p, int cnt, pcap_handler callback, u_char* user) const
{
	return pcap_loop(p, cnt, callback, user);
}

void PcapInterface::breakloop(pcap_t* p) const
{
	pcap_breakloop(p);
}

int PcapInterface::sendpacket(pcap_t* p, const u_char* buf, int size) const
{
	return pcap_sendpacket(p, buf, size);
}

} // namespace protocol
} // namespace avdecc
} // namespace la
