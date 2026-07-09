#ifndef NCMPCPP_SAMPLE_BUFFER_H
#define NCMPCPP_SAMPLE_BUFFER_H

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

#include "c/ncm_sample_buffer.h"

struct SampleBuffer
{
	typedef std::vector<int16_t>::iterator Iterator;

	SampleBuffer()
	{
		ncm_sample_buffer_init(&m_buffer);
	}

	SampleBuffer(const SampleBuffer &rhs)
	{
		ncm_sample_buffer_init(&m_buffer);
		ncm_sample_buffer_copy(
			&m_buffer, const_cast<NcmSampleBuffer *>(&rhs.m_buffer));
	}

	SampleBuffer(SampleBuffer &&rhs) noexcept
	{
		ncm_sample_buffer_init(&m_buffer);
		ncm_sample_buffer_move(&m_buffer, &rhs.m_buffer);
	}

	~SampleBuffer()
	{
		ncm_sample_buffer_destroy(&m_buffer);
	}

	SampleBuffer &operator=(const SampleBuffer &rhs)
	{
		if (this != &rhs)
			ncm_sample_buffer_copy(
				&m_buffer, const_cast<NcmSampleBuffer *>(&rhs.m_buffer));
		return *this;
	}

	SampleBuffer &operator=(SampleBuffer &&rhs) noexcept
	{
		if (this != &rhs)
			ncm_sample_buffer_move(&m_buffer, &rhs.m_buffer);
		return *this;
	}

	void put(Iterator begin, Iterator end)
	{
		int32 elems = checkedSize(static_cast<size_t>(end - begin));
		int16_t *samples = elems > 0 ? &*begin : nullptr;
		if (!ncm_sample_buffer_put(&m_buffer, samples, elems))
			throw std::out_of_range(
				"Size of the buffer is smaller than the amount of elements");
	}

	size_t get(size_t elems, std::vector<int16_t> &dest)
	{
		int32 result = ncm_sample_buffer_get_clamped(
			&m_buffer, checkedRequest(elems),
			dest.data(), checkedSize(dest.size()));
		return static_cast<size_t>(result);
	}

	void resize(size_t n)
	{
		ncm_sample_buffer_resize(&m_buffer, checkedSize(n));
	}

	void clear()
	{
		ncm_sample_buffer_clear(&m_buffer);
	}

	size_t size() const
	{
		return static_cast<size_t>(
			ncm_sample_buffer_size(
				const_cast<NcmSampleBuffer *>(&m_buffer)));
	}

	const std::vector<int16_t> &buffer() const
	{
		NcmSampleBuffer *buffer = const_cast<NcmSampleBuffer *>(&m_buffer);
		int32 capacity = ncm_sample_buffer_capacity(buffer);

		m_buffer_view.resize(static_cast<size_t>(capacity));
		ncm_sample_buffer_copy_data(
			buffer, m_buffer_view.data(), checkedSize(m_buffer_view.size()));
		return m_buffer_view;
	}

private:
	static int32 checkedSize(size_t size)
	{
		if (size > static_cast<size_t>(std::numeric_limits<int32>::max()))
			throw std::out_of_range("Sample buffer size is too large");
		return static_cast<int32>(size);
	}

	static int64 checkedRequest(size_t size)
	{
		size_t max_size =
			static_cast<size_t>(std::numeric_limits<int64>::max());
		if (size > max_size)
			return std::numeric_limits<int64>::max();
		return static_cast<int64>(size);
	}

	mutable std::vector<int16_t> m_buffer_view;
	NcmSampleBuffer m_buffer;
};

#endif // NCMPCPP_SAMPLE_BUFFER_H
