#ifndef NCMPCPP_EXEC_ITEM_H
#define NCMPCPP_EXEC_ITEM_H

#include <functional>

template <typename ItemT, typename FunctionT>
struct RunnableItem
{
	typedef ItemT Item;
	typedef std::function<FunctionT> Function;
	
	RunnableItem() { }
	template <typename Arg1, typename Arg2>
	RunnableItem(Arg1 &&opt, Arg2 &&f)
	: m_item(std::forward<Arg1>(opt)), m_f(std::forward<Arg2>(f)) { }
	
	template <typename... Args>
	typename Function::result_type run(Args&&... args) const
	{
		return m_f(std::forward<Args>(args)...);
	}
	
	Item &item() { return m_item; }
	const Item &item() const { return m_item; }
	
private:
	Item m_item;
	Function m_f;
};

#endif // NCMPCPP_EXEC_ITEM_H
