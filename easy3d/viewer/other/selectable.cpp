
#include "selectable.h"

#ifdef WIN32
#include <intrin.h>	// for __popcnt()
#define popcnt __popcnt
#else
#include <popcntintrin.h>
#define popcnt __builtin_popcount
#endif

void Selectable::set_num_primitives(std::size_t num) {
	if (num != num_primitives_) {
		//std::size_t size_needed = (num + 31) / 32;
		std::size_t size_needed = (num >> 3) + ((num & 7) ? 1 : 0);
		if (selections_.size() != size_needed)
			selections_.resize(size_needed);
		num_primitives_ = num;
	}
	clear_selection();
}


std::vector<int> Selectable::selected() const {
	std::vector<int> indices;

	if (num_primitives_ > 0) {
		for (int i = 0; i < num_primitives_; ++i) {
			if (is_selected(i))
				indices.push_back(i);
		}
	}

	return indices;
}


int Selectable::num_selected() const {
	if (selections_.empty())
		return 0;

    int count = 0;

    std::size_t lim = selections_.size() > 1 ? selections_.size() - 1 : selections_.size();
    if ((num_primitives_ % 32) == 0)
        lim++;
    for (std::size_t n = 0; n < lim; ++n) {
        count += popcnt(selections_[n]);
    }
    for (std::size_t n = 32 * lim; n < num_primitives_; ++n) {
        count += is_selected(n);
    }

    return count;
}


void Selectable::invert_selection() {
	for (std::size_t n = 0; n < selections_.size(); n++) {
		selections_[n] = ~selections_[n];
	}
}


void Selectable::clear_selection() {
	std::fill(selections_.begin(), selections_.end(), 0);
	// or
	//memset(selections_.data(), 0, selections_.size() * sizeof(uint32_t));
	//or
// 	for (std::size_t n = 0; n < selections_.size(); ++n)
// 		selections_[n] = 0;
}
