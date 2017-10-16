/*
* Copyright (C) 2016-2017, L-Acoustics and its contributors

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

/**
* @file pcapInterface.hpp
* @author Christophe Calmejane
*/

#pragma once

#include <memory>
#include <pcap.h>

namespace la
{
namespace avdecc
{
namespace protocol
{

class PcapInterface
{
public:
	PcapInterface();
	~PcapInterface();

	bool is_available() const;
	pcap_t * open_live(const char *, int, int, int, char *) const;
	int fileno(pcap_t *) const;
	void close(pcap_t *) const;
	int compile(pcap_t *, struct bpf_program *, const char *, int, bpf_u_int32) const;
	int setfilter(pcap_t *, struct bpf_program *) const;
	void freecode(struct bpf_program *) const;
	int next_ex(pcap_t *, struct pcap_pkthdr **, const u_char **) const;
	int sendpacket(pcap_t *, const u_char *, int) const;

private:
	struct pImpl;
	std::unique_ptr<pImpl> _pImpl;//{ nullptr }; NSDMI for unique_ptr not supported by gcc (for incomplete type)
};

} // namespace protocol
} // namespace avdecc
} // namespace la
