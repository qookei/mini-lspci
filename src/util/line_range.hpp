#pragma once

#include <string_view>

struct line_range {
	line_range(std::string_view sv)
	: sv_{sv}, end_iter_{cache_end_iter_()} {}

	struct iter {
		iter(const line_range *o, size_t i, size_t n)
		: o_{o}, i_{i}, newl_{o_->sv_.find('\n')}, n_{n} { }

		iter &operator++() {
			n_++;
			i_ = newl_ != std::string_view::npos ? newl_ + 1 : newl_;
			newl_ = newl_ != std::string_view::npos ? o_->sv_.find('\n', i_) : newl_;

			return *this;
		}

		auto operator*() {
			struct {
				size_t line_no;
				std::string_view line;
			} ret{n_, o_->sv_.substr(i_, newl_ - i_)};

			return ret;
		}

		bool operator==(const iter &other) const {
			return o_ == other.o_
				&& i_ == other.i_
				&& n_ == other.n_;
		}

		bool operator!=(const iter &other) const {
			return !(*this == other);
		}

	private:
		const line_range *o_;
		size_t i_;
		size_t newl_;
		size_t n_;
	};

	iter begin() const {
		return iter{this, 0, 0};
	}

	iter end() const {
		return end_iter_;
	}

private:
	iter cache_end_iter_() {
		size_t lines = 1;
		for (auto c : sv_) if (c == '\n') lines++;

		return iter{this, std::string_view::npos, lines};
	}

	std::string_view sv_;
	iter end_iter_;
};
