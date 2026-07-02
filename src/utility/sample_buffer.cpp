/***************************************************************************
 *   Copyright (C) 2008-2021 by Andrzej Rybczak                            *
 *   andrzej@rybczak.net                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include <algorithm>
#include <limits>
#include <stdexcept>

#include "utility/sample_buffer.h"

SampleBuffer::SampleBuffer()
{
	ncm_sample_buffer_init(&m_buffer);
}

SampleBuffer::SampleBuffer(const SampleBuffer &rhs)
{
	ncm_sample_buffer_init(&m_buffer);
	ncm_sample_buffer_copy(
		&m_buffer, const_cast<NcmSampleBuffer *>(&rhs.m_buffer));
}

SampleBuffer::SampleBuffer(SampleBuffer &&rhs) noexcept
{
	ncm_sample_buffer_init(&m_buffer);
	ncm_sample_buffer_move(&m_buffer, &rhs.m_buffer);
}

SampleBuffer::~SampleBuffer()
{
	ncm_sample_buffer_destroy(&m_buffer);
}

SampleBuffer &SampleBuffer::operator=(const SampleBuffer &rhs)
{
	if (this != &rhs)
		ncm_sample_buffer_copy(
			&m_buffer, const_cast<NcmSampleBuffer *>(&rhs.m_buffer));
	return *this;
}

SampleBuffer &SampleBuffer::operator=(SampleBuffer &&rhs) noexcept
{
	if (this != &rhs)
		ncm_sample_buffer_move(&m_buffer, &rhs.m_buffer);
	return *this;
}

void SampleBuffer::put(SampleBuffer::Iterator begin, SampleBuffer::Iterator end)
{
	int32 elems = checkedSize(static_cast<size_t>(end - begin));
	int16_t *samples = elems > 0 ? &*begin : nullptr;
	if (!ncm_sample_buffer_put(&m_buffer, samples, elems))
		throw std::out_of_range(
			"Size of the buffer is smaller than the amount of elements");
}

size_t SampleBuffer::get(size_t elems, std::vector<int16_t> &dest)
{
	size_t available = size();
	if (elems > available)
		elems = available;

	int32 result = ncm_sample_buffer_get(
		&m_buffer, checkedSize(elems), dest.data(), checkedSize(dest.size()));
	return static_cast<size_t>(result);
}

void SampleBuffer::resize(size_t n)
{
	ncm_sample_buffer_resize(&m_buffer, checkedSize(n));
}

void SampleBuffer::clear()
{
	ncm_sample_buffer_clear(&m_buffer);
}

size_t SampleBuffer::size() const
{
	return static_cast<size_t>(
		ncm_sample_buffer_size(const_cast<NcmSampleBuffer *>(&m_buffer)));
}

const std::vector<int16_t> &SampleBuffer::buffer() const
{
	int32 capacity = ncm_sample_buffer_capacity(
		const_cast<NcmSampleBuffer *>(&m_buffer));
	int16_t *data = ncm_sample_buffer_data(
		const_cast<NcmSampleBuffer *>(&m_buffer));

	m_buffer_view.resize(static_cast<size_t>(capacity));
	if (capacity > 0)
		std::copy(data, data + capacity, m_buffer_view.begin());
	return m_buffer_view;
}

int32 SampleBuffer::checkedSize(size_t size)
{
	if (size > static_cast<size_t>(std::numeric_limits<int32>::max()))
		throw std::out_of_range("Sample buffer size is too large");
	return static_cast<int32>(size);
}
