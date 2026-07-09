#ifndef NCMPCPP_UTILITY_SCOPED_VALUE_H
#define NCMPCPP_UTILITY_SCOPED_VALUE_H

template <typename ValueT>
struct ScopedValue
{
	ScopedValue(ValueT &ref, ValueT &&new_value)
		: m_ref(ref)
	{
		m_value = ref;
		m_ref = std::forward<ValueT>(new_value);
	}

	~ScopedValue()
	{
		m_ref = std::move(m_value);
	}

private:
	ValueT &m_ref;
	ValueT m_value;
};

#endif // NCMPCPP_UTILITY_SCOPED_VALUE_H
