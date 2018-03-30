/*
* Copyright (C) 2016-2018, L-Acoustics and its contributors

* This file is part of LA_avdecc.

* LA_avdecc is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* LA_avdecc is distributed in the hope that it will be usefu_state,
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

pcap_t * PcapInterface::open_live(const char *device, int snaplen, int promisc, int to_ms, char *ebuf) const
{
	return pcap_open_live(device, snaplen, promisc, to_ms, ebuf);
}

int PcapInterface::fileno(pcap_t *p) const
{
	return pcap_fileno(p);
}

void PcapInterface::close(pcap_t *p) const
{
	return pcap_close(p);
}

int PcapInterface::compile(pcap_t *p, struct bpf_program *fp, const char *str, int optimize, bpf_u_int32 netmask) const
{
	return pcap_compile(p, fp, str, optimize, netmask);
}

int PcapInterface::setfilter(pcap_t *p, struct bpf_program *fp) const
{
	return pcap_setfilter(p, fp);
}

void PcapInterface::freecode(bpf_program *fp) const
{
	return pcap_freecode(fp);
}

int PcapInterface::next_ex(pcap_t *p, struct pcap_pkthdr **pkt_header, const u_char **pkt_data) const
{
	return pcap_next_ex(p, pkt_header, pkt_data);
}

int PcapInterface::sendpacket(pcap_t *p, const u_char *buf, int size) const
{
	return pcap_sendpacket(p, buf, size);
}

} // namespace protocol
} // namespace avdecc
} // namespace la
