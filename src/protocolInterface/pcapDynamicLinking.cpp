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

/**
* @file pcapDynamicLinking.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/internals/logItems.hpp"

#include "pcapInterface.hpp"

#include <cassert>
#include <string>

#ifdef _WIN32
#	include <la/networkInterfaceHelper/windowsHelper.hpp>
#	include <windows.h>
#	define PCAP_LIBRARY "wpcap.dll"
#	define DL_HANDLE HMODULE
#	define DL_OPEN(x) la_dlopen(x)
#	define DL_CLOSE(x) FreeLibrary(x)
#	define DL_SYM(x, y) GetProcAddress((x), (y))
#	define DL_ERROR() la_dlerror()
#else
#	include <dlfcn.h>
#	include <signal.h>
#	ifdef __APPLE__
#		define PCAP_LIBRARY "/usr/lib/libpcap.dylib" /* Due to macOS hardened runtime, we have to specify the absolute path for the pcap library */
#	else /* !__APPLE__ */
#		if defined(OVERRIDE_PCAP_LIB_PATH)
#			define PCAP_LIBRARY OVERRIDE_PCAP_LIB_PATH
#		else // !OVERRIDE_PCAP_LIB_PATH
#			define PCAP_LIBRARY "libpcap.so"
#		endif // OVERRIDE_PCAP_LIB_PATH
#	endif /* __APPLE__ */
#	define DL_HANDLE void*
#	define DL_OPEN(x) dlopen((x), RTLD_LAZY)
#	define DL_CLOSE(x) dlclose(x)
#	define DL_SYM(x, y) dlsym((x), (y))
#	define DL_ERROR() dlerror()
#endif

#ifdef _WIN32
#	include <filesystem>

inline void la_force_npcap_dll_path()
{
	static auto already_done = false;

	if (already_done)
	{
		return;
	}

	// Compute NPCap path
	constexpr auto MaxSystemDirectoryPath = 512;
	char systemDirectoryPath[MaxSystemDirectoryPath];
	auto const len = GetSystemDirectoryA(systemDirectoryPath, MaxSystemDirectoryPath);
	if (len == 0)
	{
		return;
	}
	auto const NPCapPath = std::filesystem::path{ systemDirectoryPath } / "Npcap";

	// Check for AddDllDirectory (requires win7 or later), which is better than using SetDllDirectory
	{
		auto const Kernel32Module = GetModuleHandle("kernel32.dll");
		if (Kernel32Module != nullptr)
		{
			auto const AddDllDirectory_p = reinterpret_cast<DLL_DIRECTORY_COOKIE(WINAPI*)(PCWSTR)>(GetProcAddress(Kernel32Module, "AddDllDirectory"));
			if (AddDllDirectory_p != nullptr)
			{
				if (AddDllDirectory_p(NPCapPath.native().c_str()) != nullptr)
				{
					// No need to do this more than once
					already_done = true;
					return;
				}
			}
		}
	}

	// Try SetDllDirectory as fallback
	{
		SetDllDirectoryA(NPCapPath.string().c_str());
		// We want to call SetDllDirectory every time this function is called, as someone might have altered the paths (this is not an issue when using AddDllDirectory)
		// So we don't set already_done to true
	}
}
inline DL_HANDLE la_dlopen(LPCSTR lpLibFileName)
{
	la_force_npcap_dll_path();
	return LoadLibraryExA(lpLibFileName, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32 | LOAD_LIBRARY_SEARCH_USER_DIRS);
}
inline std::string la_dlerror()
{
	constexpr auto ErrorMessageMaxLength = 512;
	WCHAR errMessage[ErrorMessageMaxLength];

	auto const errorCode = GetLastError();
	if (FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), errMessage, ErrorMessageMaxLength, NULL) != 0)
	{
		try
		{
			return la::networkInterface::windows::wideCharToUtf8(errMessage);
		}
		catch (std::invalid_argument const&)
		{
		}
	}

	return std::to_string(errorCode);
}
#endif // _WIN32

namespace la
{
namespace avdecc
{
namespace protocol
{
using lib_version_t = const char* (*)();
using create_t = pcap_t* (*)(const char*, char*);
using set_snaplen_t = int (*)(pcap_t*, int);
using set_promisc_t = int (*)(pcap_t*, int);
using set_timeout_t = int (*)(pcap_t*, int);
#ifdef _WIN32
using setmintocopy_t = int (*)(pcap_t*, int);
#else // !_WIN32
using set_immediate_mode_t = int (*)(pcap_t*, int);
#endif // _WIN32
using activate_t = int (*)(pcap_t*);
using geterr_t = char* (*)(pcap_t*);
using fileno_t = int (*)(pcap_t*);
using close_t = void (*)(pcap_t*);
using compile_t = int (*)(pcap_t*, bpf_program*, const char*, int, bpf_u_int32);
using setfilter_t = int (*)(pcap_t*, bpf_program*);
using freecode_t = void (*)(bpf_program*);
using next_ex_t = int (*)(pcap_t*, pcap_pkthdr**, const u_char**);
using loop_t = int (*)(pcap_t*, int, pcap_handler, u_char*);
using breakloop_t = void (*)(pcap_t*);
using sendpacket_t = int (*)(pcap_t*, const u_char*, int);

struct PcapInterface::pImpl
{
	DL_HANDLE libraryHandle{ nullptr };
	lib_version_t lib_version_ptr{ nullptr };
	create_t create_ptr{ nullptr };
	set_snaplen_t set_snaplen_ptr{ nullptr };
	set_promisc_t set_promisc_ptr{ nullptr };
	set_timeout_t set_timeout_ptr{ nullptr };
#ifdef _WIN32
	setmintocopy_t setmintocopy_ptr{ nullptr };
#else // !_WIN32
	set_immediate_mode_t set_immediate_mode_ptr{ nullptr };
#endif // _WIN32
	activate_t activate_ptr{ nullptr };
	geterr_t geterr_ptr{ nullptr };
	fileno_t fileno_ptr{ nullptr };
	close_t close_ptr{ nullptr };
	compile_t compile_ptr{ nullptr };
	setfilter_t setfilter_ptr{ nullptr };
	freecode_t freecode_ptr{ nullptr };
	next_ex_t next_ex_ptr{ nullptr };
	loop_t loop_ptr{ nullptr };
	breakloop_t breakloop_ptr{ nullptr };
	sendpacket_t sendpacket_ptr{ nullptr };
};

PcapInterface::PcapInterface()
	: _pImpl(std::make_unique<pImpl>())
{
	bool foundAllFunctions{ false };
	if (auto const handle = DL_OPEN(PCAP_LIBRARY))
	{
		std::string version{};
		if (auto const lib_version_ptr = reinterpret_cast<lib_version_t>(DL_SYM(handle, "pcap_lib_version")))
		{
			version = lib_version_ptr();

			_pImpl->lib_version_ptr = lib_version_ptr;

			_pImpl->create_ptr = reinterpret_cast<create_t>(DL_SYM(handle, "pcap_create"));
			_pImpl->set_snaplen_ptr = reinterpret_cast<set_snaplen_t>(DL_SYM(handle, "pcap_set_snaplen"));
			_pImpl->set_promisc_ptr = reinterpret_cast<set_promisc_t>(DL_SYM(handle, "pcap_set_promisc"));
			_pImpl->set_timeout_ptr = reinterpret_cast<set_timeout_t>(DL_SYM(handle, "pcap_set_timeout"));
#ifdef _WIN32
			_pImpl->setmintocopy_ptr = reinterpret_cast<setmintocopy_t>(DL_SYM(handle, "pcap_setmintocopy"));
			auto const foundImmediateModeFunction = _pImpl->setmintocopy_ptr != nullptr;
#else // !_WIN32
			_pImpl->set_immediate_mode_ptr = reinterpret_cast<set_immediate_mode_t>(DL_SYM(handle, "pcap_set_immediate_mode"));
			auto const foundImmediateModeFunction = _pImpl->set_immediate_mode_ptr != nullptr;
#endif // _WIN32
			_pImpl->activate_ptr = reinterpret_cast<activate_t>(DL_SYM(handle, "pcap_activate"));
			_pImpl->geterr_ptr = reinterpret_cast<geterr_t>(DL_SYM(handle, "pcap_geterr"));
			_pImpl->fileno_ptr = reinterpret_cast<fileno_t>(DL_SYM(handle, "pcap_fileno"));
			_pImpl->close_ptr = reinterpret_cast<close_t>(DL_SYM(handle, "pcap_close"));
			_pImpl->compile_ptr = reinterpret_cast<compile_t>(DL_SYM(handle, "pcap_compile"));
			_pImpl->setfilter_ptr = reinterpret_cast<setfilter_t>(DL_SYM(handle, "pcap_setfilter"));
			_pImpl->freecode_ptr = reinterpret_cast<freecode_t>(DL_SYM(handle, "pcap_freecode"));
			_pImpl->next_ex_ptr = reinterpret_cast<next_ex_t>(DL_SYM(handle, "pcap_next_ex"));
			_pImpl->loop_ptr = reinterpret_cast<loop_t>(DL_SYM(handle, "pcap_loop"));
			_pImpl->breakloop_ptr = reinterpret_cast<breakloop_t>(DL_SYM(handle, "pcap_breakloop"));
			_pImpl->sendpacket_ptr = reinterpret_cast<sendpacket_t>(DL_SYM(handle, "pcap_sendpacket"));

			foundAllFunctions = _pImpl->create_ptr && _pImpl->set_snaplen_ptr && _pImpl->set_promisc_ptr && _pImpl->set_timeout_ptr && foundImmediateModeFunction && _pImpl->activate_ptr && _pImpl->geterr_ptr && _pImpl->fileno_ptr && _pImpl->close_ptr && _pImpl->compile_ptr && _pImpl->setfilter_ptr && _pImpl->freecode_ptr && _pImpl->next_ex_ptr && _pImpl->loop_ptr && _pImpl->breakloop_ptr && _pImpl->sendpacket_ptr;
		}

		if (foundAllFunctions)
		{
			auto const item = la::avdecc::logger::LogItemGeneric{ "Using " PCAP_LIBRARY ": " + version };
			la::avdecc::logger::Logger::getInstance().logItem(la::avdecc::logger::Level::Info, &item);
			_pImpl->libraryHandle = handle;
		}
		else
		{
			auto const item = la::avdecc::logger::LogItemGeneric{ "Cannot find all the required functions in " PCAP_LIBRARY };
			la::avdecc::logger::Logger::getInstance().logItem(la::avdecc::logger::Level::Error, &item);
			DL_CLOSE(handle);
		}
	}
	else
	{
		auto const item = la::avdecc::logger::LogItemGeneric{ std::string("Cannot load " PCAP_LIBRARY ": ") + std::string(DL_ERROR()) };
		la::avdecc::logger::Logger::getInstance().logItem(la::avdecc::logger::Level::Error, &item);
	}
}

PcapInterface::~PcapInterface()
{
	assert(_pImpl != nullptr);
	if (_pImpl->libraryHandle != nullptr)
	{
		DL_CLOSE(_pImpl->libraryHandle);
	}
}

bool PcapInterface::is_available() const
{
	assert(_pImpl != nullptr);
	return (_pImpl->libraryHandle != nullptr);
}

const char* PcapInterface::lib_version() const
{
	assert((_pImpl != nullptr) && (_pImpl->libraryHandle != nullptr) && (_pImpl->lib_version_ptr != nullptr));
	return _pImpl->lib_version_ptr();
}

pcap_t* PcapInterface::create(const char* source, char* errbuf) const
{
	assert((_pImpl != nullptr) && (_pImpl->libraryHandle != nullptr) && (_pImpl->create_ptr != nullptr));
	return _pImpl->create_ptr(source, errbuf);
}

int PcapInterface::set_snaplen(pcap_t* p, int snaplen) const
{
	assert((_pImpl != nullptr) && (_pImpl->libraryHandle != nullptr) && (_pImpl->set_snaplen_ptr != nullptr));
	return _pImpl->set_snaplen_ptr(p, snaplen);
}

int PcapInterface::set_promisc(pcap_t* p, int promisc) const
{
	assert((_pImpl != nullptr) && (_pImpl->libraryHandle != nullptr) && (_pImpl->set_promisc_ptr != nullptr));
	return _pImpl->set_promisc_ptr(p, promisc);
}

int PcapInterface::set_timeout(pcap_t* p, int to_ms) const
{
	assert((_pImpl != nullptr) && (_pImpl->libraryHandle != nullptr) && (_pImpl->set_timeout_ptr != nullptr));
	return _pImpl->set_timeout_ptr(p, to_ms);
}

#ifdef _WIN32
int PcapInterface::setmintocopy(pcap_t* p, int size) const
{
	assert((_pImpl != nullptr) && (_pImpl->libraryHandle != nullptr) && (_pImpl->setmintocopy_ptr != nullptr));
	return _pImpl->setmintocopy_ptr(p, size);
}
#else // !_WIN32
int PcapInterface::set_immediate_mode(pcap_t* p, int immediate) const
{
	assert((_pImpl != nullptr) && (_pImpl->libraryHandle != nullptr) && (_pImpl->set_immediate_mode_ptr != nullptr));
	return _pImpl->set_immediate_mode_ptr(p, immediate);
}
#endif // _WIN32

int PcapInterface::activate(pcap_t* p) const
{
	assert((_pImpl != nullptr) && (_pImpl->libraryHandle != nullptr) && (_pImpl->activate_ptr != nullptr));
	return _pImpl->activate_ptr(p);
}

char* PcapInterface::geterr(pcap_t* p) const
{
	assert((_pImpl != nullptr) && (_pImpl->libraryHandle != nullptr) && (_pImpl->geterr_ptr != nullptr));
	return _pImpl->geterr_ptr(p);
}

int PcapInterface::fileno(pcap_t* p) const
{
	assert((_pImpl != nullptr) && (_pImpl->libraryHandle != nullptr) && (_pImpl->fileno_ptr != nullptr));
	return _pImpl->fileno_ptr(p);
}

void PcapInterface::close(pcap_t* p) const
{
	assert((_pImpl != nullptr) && (_pImpl->libraryHandle != nullptr) && (_pImpl->close_ptr != nullptr));
	_pImpl->close_ptr(p);
}

int PcapInterface::compile(pcap_t* p, struct bpf_program* fp, const char* str, int optimize, bpf_u_int32 netmask) const
{
	assert((_pImpl != nullptr) && (_pImpl->libraryHandle != nullptr) && (_pImpl->compile_ptr != nullptr));
	return _pImpl->compile_ptr(p, fp, str, optimize, netmask);
}

int PcapInterface::setfilter(pcap_t* p, bpf_program* fp) const
{
	assert((_pImpl != nullptr) && (_pImpl->libraryHandle != nullptr) && (_pImpl->setfilter_ptr != nullptr));
	return _pImpl->setfilter_ptr(p, fp);
}

void PcapInterface::freecode(bpf_program* fp) const
{
	assert((_pImpl != nullptr) && (_pImpl->libraryHandle != nullptr) && (_pImpl->freecode_ptr != nullptr));
	_pImpl->freecode_ptr(fp);
}

int PcapInterface::next_ex(pcap_t* p, struct pcap_pkthdr** pkt_header, const u_char** pkt_data) const
{
	assert((_pImpl != nullptr) && (_pImpl->libraryHandle != nullptr) && (_pImpl->next_ex_ptr != nullptr));
	return _pImpl->next_ex_ptr(p, pkt_header, pkt_data);
}

int PcapInterface::loop(pcap_t* p, int cnt, pcap_handler callback, u_char* user) const
{
	assert((_pImpl != nullptr) && (_pImpl->libraryHandle != nullptr) && (_pImpl->loop_ptr != nullptr));
	return _pImpl->loop_ptr(p, cnt, callback, user);
}

void PcapInterface::breakloop(pcap_t* p) const
{
	assert((_pImpl != nullptr) && (_pImpl->libraryHandle != nullptr) && (_pImpl->breakloop_ptr != nullptr));
	_pImpl->breakloop_ptr(p);
}

int PcapInterface::sendpacket(pcap_t* p, const u_char* buf, int size) const
{
	assert((_pImpl != nullptr) && (_pImpl->libraryHandle != nullptr) && (_pImpl->sendpacket_ptr != nullptr));
	return _pImpl->sendpacket_ptr(p, buf, size);
}

} // namespace protocol
} // namespace avdecc
} // namespace la
