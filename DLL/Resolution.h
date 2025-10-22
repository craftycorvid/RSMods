#pragma once

struct Resolution {
	unsigned int width = 0;
	unsigned int height = 0;

	Resolution() = default;

	Resolution(unsigned int W, unsigned int H) : width(W), height(H) {}
};