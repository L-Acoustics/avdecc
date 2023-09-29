/*
* Copyright (C) 2016-2023, L-Acoustics and its contributors

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
* @file avdeccControllerImpl.cpp
* @author Christophe Calmejane
*/

#include "entityModelChecksum.hpp"

#include <la/avdecc/internals/endian.hpp>

#include <cstdint>
#include <type_traits>
#include <cstring> // memcpy
#include <array>
#include <sstream>
#include <iomanip>
#include <ios>

namespace la
{
namespace avdecc
{
namespace controller
{
static constexpr auto StartNode = '$';
static constexpr auto StartVirtualNode = '*';
static constexpr auto StartStaticModel = '|';

class Sha256Serializer final : public HashSerializer
{
public:
	Sha256Serializer() = default;

	virtual std::string getHash() noexcept override
	{
		// Process any remaining bytes
		finalize();

		auto packedDigest = HashType{};

		// Change endianness back to HOST
		for (auto i = 0u; i < _hash.size(); ++i)
		{
			packedDigest[i] = la::avdecc::endianSwap<la::avdecc::Endianness::HostEndian, la::avdecc::Endianness::BigEndian>(_hash[i]);
		}

		// Convert packed digest to readable string
		auto digest = std::stringstream{};
		digest << std::hex << std::uppercase;

		for (auto i = 0u; i < packedDigest.size(); ++i)
		{
			auto const* const ptr = reinterpret_cast<std::uint8_t const*>(packedDigest.data()) + i * sizeof(std::uint32_t);
			digest << std::setfill('0') << std::setw(2) << utils::forceNumeric(ptr[0]) << std::setfill('0') << std::setw(2) << utils::forceNumeric(ptr[1]) << std::setfill('0') << std::setw(2) << utils::forceNumeric(ptr[2]) << std::setfill('0') << std::setw(2) << utils::forceNumeric(ptr[3]);
		}


		return digest.str();
	}

	/** Serializes any arithmetic type (including enums) */
	template<typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value || std::is_enum<T>::value>>
	Sha256Serializer& operator<<(T const& v)
	{
		// Append to buffer
		appendBuffer(reinterpret_cast<void const*>(&v), sizeof(v));

		return *this;
	}

	/** Serializes an AvdeccFixedString (without changing endianess) */
	Sha256Serializer& operator<<(entity::model::AvdeccFixedString const& v)
	{
		appendBuffer(v.data(), v.size());
		return *this;
	}

	/** Serializes a MacAddress (without changing endianess) */
	Sha256Serializer& operator<<(networkInterface::MacAddress const& v)
	{
		appendBuffer(v.data(), v.size());
		return *this;
	}

	/** Serializes any EnumBitfield type */
	template<class Bitfield, typename T = typename std::underlying_type_t<Bitfield>>
	Sha256Serializer& operator<<(utils::EnumBitfield<Bitfield> const& v)
	{
		return operator<<(v.value());
	}

	/** Serializes a LocalizedStringReference */
	Sha256Serializer& operator<<(entity::model::LocalizedStringReference const& v)
	{
		return operator<<(v.getValue());
	}

	/** Serializes a UniqueIdentifier */
	Sha256Serializer& operator<<(UniqueIdentifier const& v)
	{
		return operator<<(v.getValue());
	}

	/** Serializes a SamplingRate */
	Sha256Serializer& operator<<(entity::model::SamplingRate const& v)
	{
		return operator<<(v.getValue());
	}

	/** Serializes a StreamFormat */
	Sha256Serializer& operator<<(entity::model::StreamFormat const& v)
	{
		return operator<<(v.getValue());
	}

	/** Serializes a ControlValueUnit */
	Sha256Serializer& operator<<(entity::model::ControlValueUnit const& v)
	{
		return operator<<(v.getValue());
	}

	/** Serializes a ControlValueType */
	Sha256Serializer& operator<<(entity::model::ControlValueType const& v)
	{
		return operator<<(v.getValue());
	}

private:
	// Private defines
	static constexpr auto Sha256BlockSize = 64u;
	static constexpr auto Sha256DigestSize = 32u;
	static constexpr auto RoundConstants = std::array<std::uint32_t, Sha256BlockSize>{ 0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814,
		0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2 };
	using HashType = std::array<std::uint32_t, Sha256BlockSize / 8u>;

	// Private methods
	void appendBuffer(void const* const buffer, size_t const size) noexcept
	{
		auto remainingSize = size;
		auto ptr = buffer;

		while (remainingSize > 0)
		{
			// Compute remaining size in the block
			auto const remainingBlockSize = static_cast<size_t>(Sha256BlockSize - _blockPos);
			auto const copyLen = remainingSize < remainingBlockSize ? remainingSize : remainingBlockSize;

			// Copy data to the block
			std::memcpy(_block.data() + _blockPos, ptr, copyLen);
			_blockPos += copyLen;

			// If block is not full, we can return
			if (_blockPos < Sha256BlockSize)
			{
				return;
			}

			// Block is full, process it
			processBlock();

			// Reset block pos
			_blockPos = 0u;

			// Update remaining bytes and ptr
			remainingSize -= copyLen;
			ptr = reinterpret_cast<std::uint8_t const*>(ptr) + copyLen;
		}
	}

	void finalize() noexcept
	{
		// Check if a block is not complete
		if (_blockPos != 0u)
		{
			// Put 0s at the end of the block
			auto const remainingBlockSize = static_cast<size_t>(Sha256BlockSize - _blockPos);
			std::memset(_block.data() + _blockPos, 0, remainingBlockSize);

			// Process the block
			processBlock();

			// Reset block pos
			_blockPos = 0u;
		}
	}

	void processBlock() noexcept
	{
		// Helper lambdas
		auto const RightShift = [](auto const value, auto const bits) noexcept
		{
			return (value >> bits);
		};
		auto const RightRotate = [](auto const value, auto const bits) noexcept
		{
			return (value >> bits) + (value << ((sizeof(value) << 3) - bits));
		};
		auto const sha256F1 = [&RightRotate](auto const value) noexcept
		{
			return RightRotate(value, 2) ^ RightRotate(value, 13) ^ RightRotate(value, 22);
		};
		auto const sha256F2 = [&RightRotate](auto const value) noexcept
		{
			return RightRotate(value, 6) ^ RightRotate(value, 11) ^ RightRotate(value, 25);
		};
		auto const sha256F3 = [&RightShift, &RightRotate](auto const value) noexcept
		{
			return RightRotate(value, 7) ^ RightRotate(value, 18) ^ RightShift(value, 3);
		};
		auto const sha256F4 = [&RightShift, &RightRotate](auto const value) noexcept
		{
			return RightRotate(value, 17) ^ RightRotate(value, 19) ^ RightShift(value, 10);
		};
		auto const sha256Ch = [](auto const v1, auto const v2, auto const v3) noexcept
		{
			return (v1 & v2) ^ (~v1 & v3);
		};
		auto const sha256Maj = [](auto const v1, auto const v2, auto const v3) noexcept
		{
			return (v1 & v2) ^ (v1 & v3) ^ (v2 & v3);
		};

		auto scheduledValues = std::array<std::uint32_t, Sha256BlockSize>{};
		auto compressedValues = HashType{};
		auto constexpr ScheduledFirstPartSize = (scheduledValues.size() / sizeof(decltype(scheduledValues)::value_type));

		// Change endianness of the block, as 32 bits array
		for (auto i = 0u; i < ScheduledFirstPartSize; ++i)
		{
			auto const* const ptr = reinterpret_cast<std::uint32_t const*>(_block.data() + (i * sizeof(std::uint32_t)));
			scheduledValues[i] = la::avdecc::endianSwap<la::avdecc::Endianness::HostEndian, la::avdecc::Endianness::BigEndian>(*ptr);
		}
		// Expand scheduledValues array
		for (auto i = ScheduledFirstPartSize; i < scheduledValues.size(); ++i)
		{
			scheduledValues[i] = sha256F4(scheduledValues[i - 2]) + scheduledValues[i - 7] + sha256F3(scheduledValues[i - 15]) + scheduledValues[i - 16];
		}
		// Init compressedValues array
		for (auto i = 0u; i < compressedValues.size(); ++i)
		{
			compressedValues[i] = _hash[i];
		}
		// Compress values
		for (auto i = 0u; i < scheduledValues.size(); ++i)
		{
			auto const temp1 = compressedValues[7] + sha256F2(compressedValues[4]) + sha256Ch(compressedValues[4], compressedValues[5], compressedValues[6]) + RoundConstants[i] + scheduledValues[i];
			auto const temp2 = sha256F1(compressedValues[0]) + sha256Maj(compressedValues[0], compressedValues[1], compressedValues[2]);
			compressedValues[7] = compressedValues[6];
			compressedValues[6] = compressedValues[5];
			compressedValues[5] = compressedValues[4];
			compressedValues[4] = compressedValues[3] + temp1;
			compressedValues[3] = compressedValues[2];
			compressedValues[2] = compressedValues[1];
			compressedValues[1] = compressedValues[0];
			compressedValues[0] = temp1 + temp2;
		}
		// Final modification
		for (auto i = 0u; i < _hash.size(); ++i)
		{
			_hash[i] += compressedValues[i];
		}
	}

	// Private members
	HashType _hash{ 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
	std::array<std::uint8_t, Sha256BlockSize> _block{};
	size_t _blockPos{ 0u };
};

ChecksumEntityModelVisitor::ChecksumEntityModelVisitor(std::uint32_t const checksumVersion) noexcept
	: _checksumVersion{ checksumVersion }
	, _serializer{ std::make_unique<Sha256Serializer>() }
{
}

std::string ChecksumEntityModelVisitor::getHash() const noexcept
{
	return _serializer->getHash();
}

// Private methods
void ChecksumEntityModelVisitor::serializeNode(model::Node const* const node) noexcept
{
	if (_checksumVersion >= 1)
	{
		if (node == nullptr)
		{
			static_cast<Sha256Serializer&>(*_serializer) << StartNode << std::uint32_t{ 0u };
		}
		else
		{
			static_cast<Sha256Serializer&>(*_serializer) << node->descriptorType;
		}
	}
}

void ChecksumEntityModelVisitor::serializeNode(model::EntityModelNode const& node) noexcept
{
	if (_checksumVersion >= 1)
	{
		static_cast<Sha256Serializer&>(*_serializer) << StartNode << node.descriptorType << node.descriptorIndex;
	}
}

void ChecksumEntityModelVisitor::serializeNode(model::VirtualNode const& node) noexcept
{
	if (_checksumVersion >= 1)
	{
		static_cast<Sha256Serializer&>(*_serializer) << StartVirtualNode << node.descriptorType << node.virtualIndex;
	}
}

void ChecksumEntityModelVisitor::serializeModel(model::ControlNode const& node) noexcept
{
	static_cast<Sha256Serializer&>(*_serializer) << StartStaticModel;
	static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.localizedDescription;
	static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.blockLatency;
	static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.controlLatency;
	static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.controlDomain;
	static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.controlType;
	static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.resetTime;
	static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.signalType;
	static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.signalIndex;
	static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.signalOutput;
	static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.controlValueType;
	static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.numberOfValues;
	static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.values.getType();
	// Missing values serialization
}

void ChecksumEntityModelVisitor::serializeModel(model::JackNode const& node) noexcept
{
	static_cast<Sha256Serializer&>(*_serializer) << StartStaticModel;
	static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.localizedDescription;
	static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.jackFlags;
	static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.jackType;
	static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.numberOfControls;
	static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.baseControl;
}

void ChecksumEntityModelVisitor::serializeModel(model::StreamPortNode const& node) noexcept
{
	static_cast<Sha256Serializer&>(*_serializer) << StartStaticModel;
	static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.clockDomainIndex;
	static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.portFlags;
	static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.numberOfControls;
	static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.baseControl;
	static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.numberOfClusters;
	static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.baseCluster;
	static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.numberOfMaps;
	static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.baseMap;
	static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.hasDynamicAudioMap;
}

#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
void ChecksumEntityModelVisitor::serializeModel(model::RedundantStreamNode const& node) noexcept
{
	static_cast<Sha256Serializer&>(*_serializer) << StartStaticModel;
	for (auto const streamIndex : node.redundantStreams)
	{
		static_cast<Sha256Serializer&>(*_serializer) << streamIndex;
	}
}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

// la::avdecc::controller::model::EntityModelVisitor overrides
void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::EntityNode const& node) noexcept
{
	if (_checksumVersion >= 1)
	{
		serializeNode(nullptr); // Parent node
		serializeNode(node); // Node itself
		static_cast<Sha256Serializer&>(*_serializer) << StartStaticModel << node.staticModel.vendorNameString << node.staticModel.modelNameString;
	}
}

void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::EntityNode const* const parent, model::ConfigurationNode const& node) noexcept
{
	if (_checksumVersion >= 1)
	{
		serializeNode(parent);
		serializeNode(node);
		static_cast<Sha256Serializer&>(*_serializer) << StartStaticModel << node.staticModel.localizedDescription;
		for (auto const& [descriptorType, count] : node.staticModel.descriptorCounts)
		{
			static_cast<Sha256Serializer&>(*_serializer) << descriptorType << count;
		}
	}
}

void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::AudioUnitNode const& node) noexcept
{
	if (_checksumVersion >= 1)
	{
		serializeNode(parent);
		serializeNode(node);
		static_cast<Sha256Serializer&>(*_serializer) << StartStaticModel;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.localizedDescription;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.clockDomainIndex;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.numberOfStreamInputPorts;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.baseStreamInputPort;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.numberOfStreamOutputPorts;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.baseStreamOutputPort;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.numberOfExternalInputPorts;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.baseExternalInputPort;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.numberOfExternalOutputPorts;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.baseExternalOutputPort;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.numberOfInternalInputPorts;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.baseInternalInputPort;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.numberOfInternalOutputPorts;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.baseInternalOutputPort;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.numberOfControls;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.baseControl;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.numberOfSignalSelectors;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.baseSignalSelector;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.numberOfMixers;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.baseMixer;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.numberOfMatrices;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.baseMatrix;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.numberOfSplitters;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.baseSplitter;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.numberOfCombiners;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.baseCombiner;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.numberOfDemultiplexers;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.baseDemultiplexer;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.numberOfMultiplexers;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.baseMultiplexer;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.numberOfTranscoders;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.baseTranscoder;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.numberOfControlBlocks;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.baseControlBlock;
		for (auto const& sr : node.staticModel.samplingRates)
		{
			static_cast<Sha256Serializer&>(*_serializer) << sr;
		}
	}
}

void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::StreamInputNode const& node) noexcept
{
	if (_checksumVersion >= 1)
	{
		serializeNode(parent);
		serializeNode(node);
		static_cast<Sha256Serializer&>(*_serializer) << StartStaticModel;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.localizedDescription;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.clockDomainIndex;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.streamFlags;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.backupTalkerEntityID_0;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.backupTalkerUniqueID_0;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.backupTalkerEntityID_1;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.backupTalkerUniqueID_1;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.backupTalkerEntityID_2;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.backupTalkerUniqueID_2;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.backedupTalkerEntityID;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.backedupTalkerUnique;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.avbInterfaceIndex;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.bufferLength;
		for (auto const& fmt : node.staticModel.formats)
		{
			static_cast<Sha256Serializer&>(*_serializer) << fmt;
		}
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
		for (auto const& rIndex : node.staticModel.redundantStreams)
		{
			static_cast<Sha256Serializer&>(*_serializer) << rIndex;
		}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
	}
}

void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::StreamOutputNode const& node) noexcept
{
	if (_checksumVersion >= 1)
	{
		serializeNode(parent);
		serializeNode(node);
		static_cast<Sha256Serializer&>(*_serializer) << StartStaticModel;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.localizedDescription;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.clockDomainIndex;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.streamFlags;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.backupTalkerEntityID_0;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.backupTalkerUniqueID_0;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.backupTalkerEntityID_1;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.backupTalkerUniqueID_1;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.backupTalkerEntityID_2;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.backupTalkerUniqueID_2;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.backedupTalkerEntityID;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.backedupTalkerUnique;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.avbInterfaceIndex;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.bufferLength;
		for (auto const& fmt : node.staticModel.formats)
		{
			static_cast<Sha256Serializer&>(*_serializer) << fmt;
		}
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
		for (auto const& rIndex : node.staticModel.redundantStreams)
		{
			static_cast<Sha256Serializer&>(*_serializer) << rIndex;
		}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
	}
}

void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::JackInputNode const& node) noexcept
{
	if (_checksumVersion >= 1)
	{
		serializeNode(parent);
		serializeNode(node);
		serializeModel(node);
	}
}

void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::JackOutputNode const& node) noexcept
{
	if (_checksumVersion >= 1)
	{
		serializeNode(parent);
		serializeNode(node);
		serializeModel(node);
	}
}

void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandParent, model::JackNode const* const parent, model::ControlNode const& node) noexcept
{
	if (_checksumVersion >= 1)
	{
		serializeNode(grandParent);
		serializeNode(parent);
		serializeNode(node);
		serializeModel(node);
	}
}

void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::AvbInterfaceNode const& node) noexcept
{
	if (_checksumVersion >= 1)
	{
		serializeNode(parent);
		serializeNode(node);
		static_cast<Sha256Serializer&>(*_serializer) << StartStaticModel;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.localizedDescription;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.macAddress;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.interfaceFlags;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.clockIdentity;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.priority1;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.clockClass;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.offsetScaledLogVariance;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.clockAccuracy;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.priority2;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.domainNumber;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.logSyncInterval;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.logAnnounceInterval;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.logPDelayInterval;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.portNumber;
	}
}

void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::ClockSourceNode const& node) noexcept
{
	if (_checksumVersion >= 1)
	{
		serializeNode(parent);
		serializeNode(node);
		static_cast<Sha256Serializer&>(*_serializer) << StartStaticModel;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.localizedDescription;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.clockSourceType;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.clockSourceLocationType;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.clockSourceLocationIndex;
	}
}

void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::MemoryObjectNode const& node) noexcept
{
	if (_checksumVersion >= 1)
	{
		serializeNode(parent);
		serializeNode(node);
		static_cast<Sha256Serializer&>(*_serializer) << StartStaticModel;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.localizedDescription;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.memoryObjectType;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.targetDescriptorType;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.targetDescriptorIndex;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.startAddress;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.maximumLength;
	}
}

void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::LocaleNode const& node) noexcept
{
	if (_checksumVersion >= 1)
	{
		serializeNode(parent);
		serializeNode(node);
		static_cast<Sha256Serializer&>(*_serializer) << StartStaticModel;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.localeID;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.numberOfStringDescriptors;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.baseStringDescriptorIndex;
	}
}

void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandParent, model::LocaleNode const* const parent, model::StringsNode const& node) noexcept
{
	if (_checksumVersion >= 1)
	{
		serializeNode(grandParent);
		serializeNode(parent);
		serializeNode(node);
		static_cast<Sha256Serializer&>(*_serializer) << StartStaticModel;
		for (auto const& str : node.staticModel.strings)
		{
			static_cast<Sha256Serializer&>(*_serializer) << str;
		}
	}
}

void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandParent, model::AudioUnitNode const* const parent, model::StreamPortInputNode const& node) noexcept
{
	if (_checksumVersion >= 1)
	{
		serializeNode(grandParent);
		serializeNode(parent);
		serializeNode(node);
		serializeModel(node);
	}
}

void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandParent, model::AudioUnitNode const* const parent, model::StreamPortOutputNode const& node) noexcept
{
	if (_checksumVersion >= 1)
	{
		serializeNode(grandParent);
		serializeNode(parent);
		serializeNode(node);
		serializeModel(node);
	}
}

void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandGrandParent, model::AudioUnitNode const* const grandParent, model::StreamPortNode const* const parent, model::AudioClusterNode const& node) noexcept
{
	if (_checksumVersion >= 1)
	{
		serializeNode(grandGrandParent);
		serializeNode(grandParent);
		serializeNode(parent);
		serializeNode(node);
		static_cast<Sha256Serializer&>(*_serializer) << StartStaticModel;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.localizedDescription;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.signalType;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.signalIndex;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.signalOutput;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.pathLatency;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.blockLatency;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.channelCount;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.format;
	}
}

void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandGrandParent, model::AudioUnitNode const* const grandParent, model::StreamPortNode const* const parent, model::AudioMapNode const& node) noexcept
{
	if (_checksumVersion >= 1)
	{
		serializeNode(grandGrandParent);
		serializeNode(grandParent);
		serializeNode(parent);
		serializeNode(node);
		static_cast<Sha256Serializer&>(*_serializer) << StartStaticModel;
		for (auto const& mapping : node.staticModel.mappings)
		{
			static_cast<Sha256Serializer&>(*_serializer) << mapping.streamIndex << mapping.streamChannel << mapping.clusterOffset << mapping.clusterChannel;
		}
	}
}

void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandGrandParent, model::AudioUnitNode const* const grandParent, model::StreamPortNode const* const parent, model::ControlNode const& node) noexcept
{
	if (_checksumVersion >= 1)
	{
		serializeNode(grandGrandParent);
		serializeNode(grandParent);
		serializeNode(parent);
		serializeNode(node);
		serializeModel(node);
	}
}

void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandParent, model::AudioUnitNode const* const parent, model::ControlNode const& node) noexcept
{
	if (_checksumVersion >= 1)
	{
		serializeNode(grandParent);
		serializeNode(parent);
		serializeNode(node);
		serializeModel(node);
	}
}

void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::ControlNode const& node) noexcept
{
	if (_checksumVersion >= 1)
	{
		serializeNode(parent);
		serializeNode(node);
		serializeModel(node);
	}
}

void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::ClockDomainNode const& node) noexcept
{
	if (_checksumVersion >= 1)
	{
		serializeNode(parent);
		serializeNode(node);
		static_cast<Sha256Serializer&>(*_serializer) << StartStaticModel;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.localizedDescription;
		for (auto const& csi : node.staticModel.clockSources)
		{
			static_cast<Sha256Serializer&>(*_serializer) << csi;
		}
	}
}

void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandParent, model::ClockDomainNode const* const parent, model::ClockSourceNode const& node) noexcept
{
	// We should not have serialized virtual parenting, so remove it from the checksum for new checksum versions
	if (_checksumVersion == 1)
	{
		serializeNode(grandParent);
		serializeNode(parent);
		serializeNode(node);
		static_cast<Sha256Serializer&>(*_serializer) << StartStaticModel;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.localizedDescription;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.clockSourceType;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.clockSourceLocationType;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.clockSourceLocationIndex;
	}
}

void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::TimingNode const& node) noexcept
{
	if (_checksumVersion >= 2)
	{
		serializeNode(parent);
		serializeNode(node);
		static_cast<Sha256Serializer&>(*_serializer) << StartStaticModel;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.localizedDescription;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.algorithm;
		for (auto const& pii : node.staticModel.ptpInstances)
		{
			static_cast<Sha256Serializer&>(*_serializer) << pii;
		}
	}
}

void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::PtpInstanceNode const& node) noexcept
{
	if (_checksumVersion >= 2)
	{
		serializeNode(parent);
		serializeNode(node);
		static_cast<Sha256Serializer&>(*_serializer) << StartStaticModel;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.localizedDescription;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.clockIdentity;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.flags;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.numberOfControls;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.baseControl;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.numberOfPtpPorts;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.basePtpPort;
	}
}

void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::TimingNode const* const /*parent*/, model::PtpInstanceNode const& /*node*/) noexcept
{
	// Do not serialize virtual parenting
}

void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandParent, model::PtpInstanceNode const* const parent, model::ControlNode const& node) noexcept
{
	if (_checksumVersion >= 2)
	{
		serializeNode(grandParent);
		serializeNode(parent);
		serializeNode(node);
		serializeModel(node);
	}
}

void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandParent, model::PtpInstanceNode const* const parent, model::PtpPortNode const& node) noexcept
{
	if (_checksumVersion >= 2)
	{
		serializeNode(grandParent);
		serializeNode(parent);
		serializeNode(node);
		static_cast<Sha256Serializer&>(*_serializer) << StartStaticModel;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.localizedDescription;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.portNumber;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.portType;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.flags;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.avbInterfaceIndex;
		static_cast<Sha256Serializer&>(*_serializer) << node.staticModel.profileIdentifier;
	}
}

void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandGrandParent*/, model::TimingNode const* const /*grandParent*/, model::PtpInstanceNode const* const /*parent*/, model::ControlNode const& /*node*/) noexcept
{
	// Do not serialize virtual parenting
}

void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandGrandParent*/, model::TimingNode const* const /*grandParent*/, model::PtpInstanceNode const* const /*parent*/, model::PtpPortNode const& /*node*/) noexcept
{
	// Do not serialize virtual parenting
}

#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::RedundantStreamInputNode const& node) noexcept
{
	if (_checksumVersion >= 1)
	{
		serializeNode(parent);
		serializeNode(node);
		serializeModel(node);
	}
}
void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::RedundantStreamOutputNode const& node) noexcept
{
	if (_checksumVersion >= 1)
	{
		serializeNode(parent);
		serializeNode(node);
		serializeModel(node);
	}
}

void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandParent, model::RedundantStreamNode const* const parent, model::StreamInputNode const& node) noexcept
{
	if (_checksumVersion >= 1)
	{
		serializeNode(grandParent);
		serializeNode(parent);
		serializeNode(node);
	}
}

void ChecksumEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandParent, model::RedundantStreamNode const* const parent, model::StreamOutputNode const& node) noexcept
{
	if (_checksumVersion >= 1)
	{
		serializeNode(grandParent);
		serializeNode(parent);
		serializeNode(node);
	}
}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

} // namespace controller
} // namespace avdecc
} // namespace la
