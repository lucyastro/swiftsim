/*******************************************************************************
 * This file is part of SWIFT.
 * Copyright (c) 2018 Matthieu Schaller (schaller@strw.leidenuniv.nl)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *******************************************************************************/
#ifndef SWIFT_RANDOM_H
#define SWIFT_RANDOM_H

/* COde configuration */
#include "../config.h"

/* Standard header */
#include <stdlib.h>

/**
 * @brief The categories of random number generated.
 */
enum random_number_type {
  random_number_star_formation = 0,
  random_number_stellar_feedback,
  random_number_stellar_enrichment,
  random_number_BH_feedback
};

/**
 * @brief Returns a pseudo-random number in the range [0, 1[.
 *
 * We generate numbers that are always reproducible for a given particle ID and
 * simulation time. If more than one number per time-step per particle is
 * needed, additional randomness can be obtained by using the type argument.
 *
 * @param id The ID of the particle for which to generate a number.
 * @param ti_current The time (on the time-line) for which to generate a number.
 * @param type The #random_number_type to generate.
 * @return a random number in the interval [0, 1.[.
 */
INLINE static double random_unit_interval(const long long int id,
                                          const integertime_t ti_current,
                                          const enum random_number_type type) {

  /* Range used for the seeds. Best if prime */
  static const long long seed_range = 131071LL;
  static const double RAND_MAX_inv = 1. / ((double)RAND_MAX);

  /* Calculate the seed */
  unsigned int seed = (id + ti_current + (long long)type) % seed_range;

  /* Generate a random number between 0 and 1. */
  return rand_r(&seed) / RAND_MAX_inv;
}

#endif /* SWIFT_RANDOM_H */
