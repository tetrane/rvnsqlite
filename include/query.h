#pragma once

#include <experimental/optional>
#include "sqlite.h"

namespace reven {
namespace sqlite {

// T: type of objects produced by the query
// F: function of signature f(Statement&) -> T
template<typename T, typename F>
class Query {
public:

	Query(sqlite::Statement stmt, F f);

	class Iterator {
	public:
		using value_type = T;
		using reference = const value_type&;
		using pointer = const value_type*;
		using iterator_category = std::input_iterator_tag;
		using difference_type = std::ptrdiff_t;

		Iterator() : query_(nullptr) {}
		Iterator(Query* query) : query_(query) {}

		reference operator*() const { return *query_->current_; }
		pointer operator->() const { return &(**this); }

		Iterator& operator++();
		Iterator operator++(int) const { return ++Iterator(*this); }

		bool operator==(const Iterator& other) const { return query_ == other.query_; }
		bool operator!=(const Iterator& other) const { return not (*this == other); }
	private:
		Query* query_;
	};

	Iterator begin() { return current(); }
	Iterator current() {
		if (finished()) {
			return end();
		}
		return {this};
	}
	Iterator end() { return {}; }
	bool finished() const {
		return not fetch_stmt_;
	}

private:
	std::experimental::optional<T> current_;
	std::experimental::optional<reven::sqlite::Statement> fetch_stmt_;
	F f_;
};

template<typename T, typename F>
typename Query<T, F>::Iterator& Query<T, F>::Iterator::operator++()
{
	auto& stmt = *query_->fetch_stmt_;
	if (stmt.step() == sqlite::Statement::StepResult::Row) {
		query_->current_ = query_->f_(stmt);
	} else {
		query_->fetch_stmt_ = {};
		query_ = nullptr;
	}
	return *this;
}

template<typename T, typename F>
Query<T, F>::Query(sqlite::Statement stmt, F f) :
    fetch_stmt_(std::move(stmt)),
    f_(std::move(f))
{
	if (fetch_stmt_->step() == sqlite::Statement::StepResult::Row) {
		current_ = f_(*fetch_stmt_);
	} else {
		fetch_stmt_ = {};
	}
}

}} // namespace reven::sqlite
