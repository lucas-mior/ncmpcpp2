#ifndef NCMPCPP_UTILITY_SHARED_RESOURCE_H
#define NCMPCPP_UTILITY_SHARED_RESOURCE_H

#include <mutex>

template <typename ResourceT>
struct Shared
{
	struct Resource
	{
		Resource(std::mutex &mutex, ResourceT &resource)
			: m_lock(std::unique_lock<std::mutex>(mutex)), m_resource(resource)
		{ }

		ResourceT &operator*() { return m_resource; }
		const ResourceT &operator*() const { return m_resource; }

		ResourceT *operator->() { return &m_resource; }
		const ResourceT *operator->() const { return &m_resource; }

	private:
		std::unique_lock<std::mutex> m_lock;
		ResourceT &m_resource;
	};

	Shared(){ }

	template <typename ValueT>
	Shared(ValueT &&value)
		: m_resource(std::forward<ValueT>(value))
	{ }

	Resource acquire() { return Resource(m_mutex, m_resource); }

private:
	std::mutex m_mutex;
	ResourceT m_resource;
};

#endif // NCMPCPP_UTILITY_SHARED_RESOURCE_H
