/*
 * Copyright 2011, Jacques Fortier
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the “Software”), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdexcept>

#include "cobsSerialization.hpp"

namespace la
{
namespace avdecc
{
namespace protocol
{
namespace cobs
{
/**
 * COBS encodes a message
 * @param input [in] pointer to the raw message
 * @param input_length [in] the length of the raw message
 * @param output [out] pointer to the output encode buffer
 * @return the number of bytes written to "output".
 */
std::size_t encode(const std::uint8_t* input, std::size_t input_length, std::uint8_t* output) noexcept
{
	std::size_t read_index = 0, write_index = 1;
	std::size_t code_index = 0;
	std::uint8_t code = 1;

	while (read_index < input_length)
	{
		if (input[read_index] == 0)
		{
			output[code_index] = code;
			code = 1;
			code_index = write_index++;
			read_index++;
		}
		else
		{
			output[write_index++] = input[read_index++];
			code++;
			if (code == 0xFF)
			{
				output[code_index] = code;
				code = 1;
				code_index = write_index++;
			}
		}
	}

	output[code_index] = code;

	return write_index;
}

/**
 * Decodes a COBS encoded message
 * @param input [in] pointer to the COBS encoded message
 * @param input_length [in] the length of the COBS encoded message
 * @param output [in] pointer to the decode buffer
 * @param output_length [in] length of the decode buffer
 * @return the number of bytes written to "output" if "input" was successfully
 * unstuffed, and 0 if there was an error unstuffing "input".
 */
std::size_t decode(const std::uint8_t* input, std::size_t input_length, std::uint8_t* output, std::size_t output_length)
{
	std::size_t read_index = 0, write_index = 0;
	std::uint8_t code, i;

	while (read_index < input_length)
	{
		code = input[read_index];

		if (read_index + code > input_length && code != 1)
		{
			return 0;
		}

		read_index++;

		for (i = 1; i < code; i++)
		{
			if (write_index == output_length)
			{
				throw std::invalid_argument("Not enough room to decode");
			}
			output[write_index++] = input[read_index++];
		}
		if (code != 0xFF && read_index != input_length)
		{
			if (write_index == output_length)
			{
				throw std::invalid_argument("Not enough room to decode");
			}
			output[write_index++] = '\0';
		}
	}

	return write_index;
}

} // namespace cobs
} // namespace protocol
} // namespace avdecc
} // namespace la
