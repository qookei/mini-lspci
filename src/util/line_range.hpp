#pragma once

#include <string_view>

struct line_range {
	line_range(std::string_view sv)
	: sv_{sv} {}

	struct iter {
		iter(const line_range *o, size_t i)
		: o_{o}, i_{i}, newl_{o_->sv_.find('\n')} { }

		iter &operator++() {
			i_ = newl_ != std::string_view::npos ? newl_ + 1 : newl_;
			newl_ = newl_ != std::string_view::npos ? o_->sv_.find('\n', i_) : newl_;

			return *this;
		}

		std::string_view operator*() {
			return o_->sv_.substr(i_, newl_ - i_);
		}

		bool operator==(const iter &other) const {
			return o_ == other.o_ && i_ == other.i_;
		}

	private:
		const line_range *o_;
		size_t i_;
		size_t newl_;
	};

	iter begin() const {
		return iter{this, 0};
	}

	iter end() const {
		return iter{this, std::string_view::npos};
	}

private:
	std::string_view sv_;
};
