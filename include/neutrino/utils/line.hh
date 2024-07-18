//
// Created by igor on 7/18/24.
//

#ifndef NEUTRINO_UTILS_LINE_HH
#define NEUTRINO_UTILS_LINE_HH

namespace neutrino::utils {
	namespace detail {
		template<typename T>
		int sgn(T x) {
			if (x < 0) {
				return -1;
			} else if (x == 0) {
				return 0;
			} else {
				return 1;
			}
		}

		template<typename T>
		T abs(T x) {
			if (x < 0) {
				return -x;
			}
			return x;
		}
	}

	template<typename Callable>
	void line(int x1, int y1, int x2, int y2, const Callable& plot) {
		int d;

		int dx = x2 - x1;
		const int sx = detail::sgn(dx);
		dx = detail::abs(dx) << 1;

		int dy = y2 - y1;
		const int sy = detail::sgn(dy);
		dy = detail::abs(dy) << 1;

		int x = x1;
		int y = y1;
		if (dx > dy) {
			d = -(dx >> 1);
			while (true) {
				plot(x, y);
				if (x == x2) {
					break;
				}
				d += dy;
				if (d >= 0) {
					y += sy;
					d -= dx;
				}
				x += sx;
			}
		} else {
			d = -(dy >> 1);
			while (true) {
				plot(x, y);
				if (y == y2) {
					break;
				}
				d += dx;
				if (d >= 0) {
					x += sx;
					d -= dy;
				}
				y += sy;
			}
		}
	}
}

#endif
