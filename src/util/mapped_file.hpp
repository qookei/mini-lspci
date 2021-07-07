#pragma once

#include <string_view>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

struct mapped_file {
	friend void swap(mapped_file &a, mapped_file &b) {
		using std::swap;
		swap(a.data_, b.data_);
		swap(a.size_, b.size_);
	}

	mapped_file() = default;

	mapped_file(const char *path) {
		int fd = open(path, O_RDONLY);
		if (fd < 0) {
			perror("open");
			return;
		}

		struct stat st;
		if (fstat(fd, &st)) {
			perror("stat");
			return;
		}

		auto ptr = mmap(nullptr, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
		if (ptr == MAP_FAILED) {
			perror("mmap");
			return;
		}

		close(fd);

		data_ = ptr;
		size_ = st.st_size;
	}

	~mapped_file() {
		munmap(data_, size_);
	}

	mapped_file(const mapped_file &) = delete;
	mapped_file &operator=(const mapped_file &) = delete;

	mapped_file(mapped_file &&other) {
		swap(*this, other);
	}

	mapped_file &operator=(mapped_file &&other) {
		swap(*this, other);
		return *this;
	}

	std::string_view as_string_view() const {
		return {static_cast<const char *>(data_), size_};
	}

	operator bool () const {
		return data_;
	}

private:
	void *data_ = nullptr;
	size_t size_ = 0;
};
